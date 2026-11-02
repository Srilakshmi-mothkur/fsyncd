#ifndef FSYNCD_SLICE_H
#define FSYNCD_SLICE_H

#include <stddef.h>
#include <stdint.h>

// A pointer to some bytes and their length. A slice does not own the bytes and never
// frees them, the owner has to keep them alive for as long as the slice is used.
// Keys and values are slices, not C strings, because they can hold any byte, even 0.
typedef struct {
    const uint8_t *data;
    size_t len;
} slice_t;

// Slice over a C string, without its ending 0 byte. Nothing is copied.
slice_t slice_from_str(const char *s);

// Compares byte by byte. Returns <0 if a sorts before b, 0 if equal, >0 if a sorts
// after b. When one is the start of the other, the shorter one sorts first.
// fsyncd keeps keys in this order, in memory and on disk.
int slice_compare(slice_t a, slice_t b);

#endif
