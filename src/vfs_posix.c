#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "vfs.h"

struct vfs_file {
    int fd;
};

// wraps an open fd in a vfs_file_t, closes the fd if malloc fails
static status_t new_file(int fd, vfs_file_t **out) {
    vfs_file_t *f = malloc(sizeof *f);
    if (f == NULL) {
        close(fd);
        return ST_NO_MEMORY;
    }
    f->fd = fd;
    *out = f;
    return ST_OK;
}

static status_t posix_open_write(vfs_t *vfs, const char *path, vfs_file_t **out) {
    (void)vfs;
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (fd < 0) return ST_IO_ERROR;
    return new_file(fd, out);
}

static status_t posix_append(vfs_t *vfs, vfs_file_t *f, slice_t data) {
    (void)vfs;
    const uint8_t *p = data.data;
    size_t left = data.len;
    // write can write less than asked, keep going until all of it is written
    while (left > 0) {
        ssize_t n = write(f->fd, p, left);
        if (n < 0) {
            if (errno == EINTR) continue;
            return ST_IO_ERROR;
        }
        p += n;
        left -= (size_t)n;
    }
    return ST_OK;
}

static int sync_fd(int fd) {
#ifdef __APPLE__
    // On mac fsync only hands the data to the drive, which can keep it in its own
    // cache and lose it on power loss. F_FULLFSYNC waits until the drive has stored
    // it. Some file systems don't support it, then fsync is the best we can do.
    if (fcntl(fd, F_FULLFSYNC) == 0) return 0;
    return fsync(fd);
#else
    // fdatasync skips metadata that isn't needed to read the data back, like the
    // last modified time, so it is a bit faster than fsync
    return fdatasync(fd);
#endif
}

static status_t posix_sync(vfs_t *vfs, vfs_file_t *f) {
    (void)vfs;
    return sync_fd(f->fd) == 0 ? ST_OK : ST_IO_ERROR;
}

static status_t posix_close(vfs_t *vfs, vfs_file_t *f) {
    (void)vfs;
    int rc = close(f->fd);
    free(f);
    return rc == 0 ? ST_OK : ST_IO_ERROR;
}

static status_t posix_open_read(vfs_t *vfs, const char *path, vfs_file_t **out) {
    (void)vfs;
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) return errno == ENOENT ? ST_NOT_FOUND : ST_IO_ERROR;
    return new_file(fd, out);
}

static status_t posix_read_at(vfs_t *vfs, vfs_file_t *f, uint64_t offset, size_t n,
                              uint8_t *out, size_t *got) {
    (void)vfs;
    size_t done = 0;
    // pread can also read less than asked, keep going until n bytes or the end
    while (done < n) {
        ssize_t r = pread(f->fd, out + done, n - done, (off_t)(offset + done));
        if (r < 0) {
            if (errno == EINTR) continue;
            return ST_IO_ERROR;
        }
        if (r == 0) break;   // end of file
        done += (size_t)r;
    }
    *got = done;
    return ST_OK;
}

static status_t posix_size(vfs_t *vfs, vfs_file_t *f, uint64_t *size) {
    (void)vfs;
    struct stat st;
    if (fstat(f->fd, &st) != 0) return ST_IO_ERROR;
    *size = (uint64_t)st.st_size;
    return ST_OK;
}

static status_t posix_rename(vfs_t *vfs, const char *from, const char *to) {
    (void)vfs;
    if (rename(from, to) == 0) return ST_OK;
    return errno == ENOENT ? ST_NOT_FOUND : ST_IO_ERROR;
}

static status_t posix_remove(vfs_t *vfs, const char *path) {
    (void)vfs;
    if (unlink(path) == 0) return ST_OK;
    return errno == ENOENT ? ST_NOT_FOUND : ST_IO_ERROR;
}

static status_t posix_make_dir(vfs_t *vfs, const char *path) {
    (void)vfs;
    if (mkdir(path, 0755) == 0 || errno == EEXIST) return ST_OK;
    return ST_IO_ERROR;
}

static status_t posix_sync_dir(vfs_t *vfs, const char *path) {
    (void)vfs;
    int fd = open(path, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fd < 0) return errno == ENOENT ? ST_NOT_FOUND : ST_IO_ERROR;
#ifdef __APPLE__
    int rc = sync_fd(fd);
#else
    // the Linux fsync man page says a new name is only safe after fsync on the folder
    int rc = fsync(fd);
#endif
    close(fd);
    return rc == 0 ? ST_OK : ST_IO_ERROR;
}

vfs_t *vfs_posix(void) {
    static vfs_t posix = {
        .open_write = posix_open_write,
        .append = posix_append,
        .sync = posix_sync,
        .close = posix_close,
        .open_read = posix_open_read,
        .read_at = posix_read_at,
        .size = posix_size,
        .rename = posix_rename,
        .remove = posix_remove,
        .make_dir = posix_make_dir,
        .sync_dir = posix_sync_dir,
        .ctx = NULL,
    };
    return &posix;
}
