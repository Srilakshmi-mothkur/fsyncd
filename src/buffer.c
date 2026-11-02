#include <stdlib.h>
#include <string.h>

#include "buffer.h"

void buffer_init(buffer_t *b) {
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

void buffer_destroy(buffer_t *b) {
    free(b->data);
    buffer_init(b);
}

void buffer_clear(buffer_t *b) {
    b->len = 0;
}

static status_t buffer_reserve(buffer_t *b, size_t extra) {
    if (b->cap - b->len >= extra) return ST_OK;

    size_t new_cap = b->cap ? b->cap : 64;
    while (new_cap - b->len < extra) new_cap *= 2;

    // If realloc fails it returns NULL and the old memory is still ours,
    // so don't overwrite b->data until we know it worked.
    uint8_t *tmp = realloc(b->data, new_cap);
    if (tmp == NULL) return ST_NO_MEMORY;
    b->data = tmp;
    b->cap = new_cap;
    return ST_OK;
}

status_t buffer_append(buffer_t *b, const void *src, size_t n) {
    status_t st = buffer_reserve(b, n);
    if (st != ST_OK) return st;
    // memcpy with a NULL pointer is undefined even for 0 bytes
    if (n > 0) memcpy(b->data + b->len, src, n);
    b->len += n;
    return ST_OK;
}

slice_t buffer_slice(const buffer_t *b) {
    slice_t s = { b->data, b->len };
    return s;
}
