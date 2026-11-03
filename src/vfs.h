#ifndef FSYNCD_VFS_H
#define FSYNCD_VFS_H

#include "slice.h"
#include "status.h"

// An open file. Each backend decides what is inside, the rest of fsyncd only
// passes it around.
typedef struct vfs_file vfs_file_t;

typedef struct vfs vfs_t;

// Every file operation in fsyncd goes through a vfs, never straight to the OS.
// Tests can pass a fake vfs that keeps every write in memory, then pretend the
// power went out after any one of them and check fsyncd still recovers.
struct vfs {
    // Creates the file, or empties it if it exists, and opens it for writing.
    status_t (*open_write)(vfs_t *vfs, const char *path, vfs_file_t **out);

    // Adds data at the end. It is not safe on disk until sync returns ST_OK.
    status_t (*append)(vfs_t *vfs, vfs_file_t *f, slice_t data);

    // Returns once everything appended so far is stored on disk.
    status_t (*sync)(vfs_t *vfs, vfs_file_t *f);

    // Closes the file and frees f, even when it returns an error.
    status_t (*close)(vfs_t *vfs, vfs_file_t *f);

    // Opens a file that already exists. Returns ST_NOT_FOUND if it doesn't.
    status_t (*open_read)(vfs_t *vfs, const char *path, vfs_file_t **out);

    // Reads up to n bytes starting at offset into out, and sets *got to how many
    // were read. *got is less than n only when the file ends first.
    status_t (*read_at)(vfs_t *vfs, vfs_file_t *f, uint64_t offset, size_t n,
                        uint8_t *out, size_t *got);

    // Sets *size to the length of the file in bytes.
    status_t (*size)(vfs_t *vfs, vfs_file_t *f, uint64_t *size);

    // Gives a file a new name in one step, replacing to if it exists. Anyone
    // looking sees the old file or the new one, never a mix of both. The rename
    // is not safe on disk until sync_dir is called on the folder.
    status_t (*rename)(vfs_t *vfs, const char *from, const char *to);

    // Deletes a file. Returns ST_NOT_FOUND if it doesn't exist.
    status_t (*remove)(vfs_t *vfs, const char *path);

    // Creates a folder. A folder that already exists is fine.
    status_t (*make_dir)(vfs_t *vfs, const char *path);

    // Makes new, renamed and removed files in the folder safe on disk. Syncing a
    // file only saves its bytes, the folder keeps the list of names.
    status_t (*sync_dir)(vfs_t *vfs, const char *path);

    void *ctx;
};

// The backend that uses real files. It is shared and never freed.
vfs_t *vfs_posix(void);

#endif
