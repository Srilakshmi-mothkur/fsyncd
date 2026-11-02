#ifndef FSYNCD_BUFFER_H
#define FSYNCD_BUFFER_H

#include <stddef.h>
#include <stdint.h>

#include "slice.h"
#include "status.h"

// A byte array that grows as bytes are added. The buffer owns its memory,
// call buffer_destroy when done with it.
typedef struct {
    uint8_t *data;
    size_t len;   // bytes in use
    size_t cap;   // bytes allocated
} buffer_t;

void buffer_init(buffer_t *b);
void buffer_destroy(buffer_t *b);

// Sets len to 0 but keeps the memory, so the next record can reuse it.
void buffer_clear(buffer_t *b);

// Adds n bytes from src at the end. Growing can move data, so any slice taken
// before the append may point at freed memory after it.
status_t buffer_append(buffer_t *b, const void *src, size_t n);

// Slice over the bytes in use. Valid until the next append, clear or destroy.
slice_t buffer_slice(const buffer_t *b);

#endif
