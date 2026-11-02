#ifndef FSYNCD_STATUS_H
#define FSYNCD_STATUS_H

// Every fsyncd function that can fail returns one of these.
// ST_OK is 0, so if (st) is true only when something went wrong.
typedef enum {
    ST_OK = 0,
    ST_NOT_FOUND,    // the key does not exist, callers usually expect this
    ST_CORRUPTION,   // bytes read from disk are wrong, like a bad checksum or a length
                     // past the end of the file, and can't be trusted
    ST_IO_ERROR,     // the OS failed a read, write or sync. After a failed sync the data
                     // may already be lost, so writes have to stop.
    ST_NO_MEMORY,
} status_t;

const char *status_str(status_t st);

#endif
