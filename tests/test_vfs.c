#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "test.h"
#include "vfs.h"

static char dir[] = "/tmp/fsyncd-XXXXXX";

// reads a whole small file with plain stdio, to check what the vfs wrote
static size_t read_back(const char *path, char *out, size_t cap) {
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) return 0;
    size_t n = fread(out, 1, cap, fp);
    fclose(fp);
    return n;
}

static void test_write_and_sync(void) {
    vfs_t *vfs = vfs_posix();
    char path[256];
    snprintf(path, sizeof path, "%s/wal.log", dir);

    vfs_file_t *f = NULL;
    CHECK(vfs->open_write(vfs, path, &f) == ST_OK);
    CHECK(vfs->append(vfs, f, slice_from_str("apple=")) == ST_OK);
    CHECK(vfs->append(vfs, f, slice_from_str("red")) == ST_OK);
    CHECK(vfs->sync(vfs, f) == ST_OK);
    CHECK(vfs->close(vfs, f) == ST_OK);

    char got[64];
    size_t n = read_back(path, got, sizeof got);
    CHECK(n == 9);
    CHECK(memcmp(got, "apple=red", 9) == 0);
    unlink(path);
}

static void test_open_write_empties_file(void) {
    vfs_t *vfs = vfs_posix();
    char path[256];
    snprintf(path, sizeof path, "%s/old.log", dir);

    vfs_file_t *f = NULL;
    CHECK(vfs->open_write(vfs, path, &f) == ST_OK);
    CHECK(vfs->append(vfs, f, slice_from_str("old data")) == ST_OK);
    CHECK(vfs->close(vfs, f) == ST_OK);

    CHECK(vfs->open_write(vfs, path, &f) == ST_OK);
    CHECK(vfs->close(vfs, f) == ST_OK);

    char got[64];
    CHECK(read_back(path, got, sizeof got) == 0);
    unlink(path);
}

static void test_open_write_missing_dir(void) {
    vfs_t *vfs = vfs_posix();
    char path[256];
    snprintf(path, sizeof path, "%s/missing/wal.log", dir);

    vfs_file_t *f = NULL;
    CHECK(vfs->open_write(vfs, path, &f) == ST_IO_ERROR);
}

// writes text to a file through the vfs, for the read tests
static void write_file(vfs_t *vfs, const char *path, const char *text) {
    vfs_file_t *f = NULL;
    CHECK(vfs->open_write(vfs, path, &f) == ST_OK);
    CHECK(vfs->append(vfs, f, slice_from_str(text)) == ST_OK);
    CHECK(vfs->close(vfs, f) == ST_OK);
}

static void test_read_at(void) {
    vfs_t *vfs = vfs_posix();
    char path[256];
    snprintf(path, sizeof path, "%s/wal.log", dir);
    write_file(vfs, path, "apple=red");

    vfs_file_t *f = NULL;
    CHECK(vfs->open_read(vfs, path, &f) == ST_OK);

    uint64_t size = 0;
    CHECK(vfs->size(vfs, f, &size) == ST_OK);
    CHECK(size == 9);

    uint8_t buf[16];
    size_t got = 0;
    CHECK(vfs->read_at(vfs, f, 0, 5, buf, &got) == ST_OK);
    CHECK(got == 5 && memcmp(buf, "apple", 5) == 0);

    CHECK(vfs->read_at(vfs, f, 6, 3, buf, &got) == ST_OK);
    CHECK(got == 3 && memcmp(buf, "red", 3) == 0);

    CHECK(vfs->close(vfs, f) == ST_OK);
    unlink(path);
}

// asking for more than is left gives what is there, not an error
static void test_read_past_end(void) {
    vfs_t *vfs = vfs_posix();
    char path[256];
    snprintf(path, sizeof path, "%s/wal.log", dir);
    write_file(vfs, path, "apple=red");

    vfs_file_t *f = NULL;
    CHECK(vfs->open_read(vfs, path, &f) == ST_OK);

    uint8_t buf[16];
    size_t got = 0;
    CHECK(vfs->read_at(vfs, f, 6, 10, buf, &got) == ST_OK);
    CHECK(got == 3);

    CHECK(vfs->read_at(vfs, f, 100, 4, buf, &got) == ST_OK);
    CHECK(got == 0);

    CHECK(vfs->close(vfs, f) == ST_OK);
    unlink(path);
}

static void test_open_read_missing_file(void) {
    vfs_t *vfs = vfs_posix();
    char path[256];
    snprintf(path, sizeof path, "%s/missing.log", dir);

    vfs_file_t *f = NULL;
    CHECK(vfs->open_read(vfs, path, &f) == ST_NOT_FOUND);
}

// the swap fsyncd uses for CURRENT, write a temp file then rename it over
static void test_rename_replaces(void) {
    vfs_t *vfs = vfs_posix();
    char current[256], tmp[256];
    snprintf(current, sizeof current, "%s/CURRENT", dir);
    snprintf(tmp, sizeof tmp, "%s/CURRENT.tmp", dir);

    write_file(vfs, current, "MANIFEST-1");
    write_file(vfs, tmp, "MANIFEST-2");
    CHECK(vfs->rename(vfs, tmp, current) == ST_OK);
    CHECK(vfs->sync_dir(vfs, dir) == ST_OK);

    char got[64];
    size_t n = read_back(current, got, sizeof got);
    CHECK(n == 10 && memcmp(got, "MANIFEST-2", 10) == 0);

    vfs_file_t *f = NULL;
    CHECK(vfs->open_read(vfs, tmp, &f) == ST_NOT_FOUND);
    unlink(current);
}

static void test_remove(void) {
    vfs_t *vfs = vfs_posix();
    char path[256];
    snprintf(path, sizeof path, "%s/old.log", dir);
    write_file(vfs, path, "old data");

    CHECK(vfs->remove(vfs, path) == ST_OK);
    vfs_file_t *f = NULL;
    CHECK(vfs->open_read(vfs, path, &f) == ST_NOT_FOUND);
    CHECK(vfs->remove(vfs, path) == ST_NOT_FOUND);
}

static void test_make_dir(void) {
    vfs_t *vfs = vfs_posix();
    char db[256], path[256];
    snprintf(db, sizeof db, "%s/mydb", dir);
    snprintf(path, sizeof path, "%s/wal.log", db);

    CHECK(vfs->make_dir(vfs, db) == ST_OK);
    CHECK(vfs->make_dir(vfs, db) == ST_OK);
    write_file(vfs, path, "apple=red");
    CHECK(vfs->sync_dir(vfs, db) == ST_OK);

    unlink(path);
    rmdir(db);
}

static void test_sync_missing_dir(void) {
    vfs_t *vfs = vfs_posix();
    char path[256];
    snprintf(path, sizeof path, "%s/missing", dir);
    CHECK(vfs->sync_dir(vfs, path) == ST_NOT_FOUND);
}

int main(void) {
    if (mkdtemp(dir) == NULL) {
        perror("mkdtemp");
        return 1;
    }
    RUN_TEST(test_write_and_sync);
    RUN_TEST(test_open_write_empties_file);
    RUN_TEST(test_open_write_missing_dir);
    RUN_TEST(test_read_at);
    RUN_TEST(test_read_past_end);
    RUN_TEST(test_open_read_missing_file);
    RUN_TEST(test_rename_replaces);
    RUN_TEST(test_remove);
    RUN_TEST(test_make_dir);
    RUN_TEST(test_sync_missing_dir);
    rmdir(dir);
    return TEST_RESULT();
}
