#ifndef FSYNCD_SKIPLIST_H
#define FSYNCD_SKIPLIST_H

#include <stdbool.h>
#include <stddef.h>

#include "slice.h"
#include "status.h"

// Sorted keys in memory, the memtable is built on this.
// Keys and values are copied in, so the caller can free theirs after a put.
typedef struct skiplist skiplist_t;
typedef struct skiplist_node skiplist_node_t;

status_t skiplist_new(skiplist_t **out);
void skiplist_free(skiplist_t *sl);

// Replaces the value if the key is already there.
status_t skiplist_put(skiplist_t *sl, slice_t key, slice_t value);

// Looks up key, returns ST_NOT_FOUND if it's not there.
// value is not a copy, it points at the bytes inside the skiplist. If you need
// it after putting the same key again or after skiplist_free, copy it first.
status_t skiplist_get(const skiplist_t *sl, slice_t key, slice_t *value);

size_t skiplist_count(const skiplist_t *sl);

// Size of all keys and values, the memtable uses it to know when to flush.
size_t skiplist_bytes(const skiplist_t *sl);

// Walks keys in sorted order. Don't put while walking.
typedef struct {
    const skiplist_node_t *node;
} skiplist_iter_t;

void skiplist_iter_init(skiplist_iter_t *it, const skiplist_t *sl);
bool skiplist_iter_valid(const skiplist_iter_t *it);
void skiplist_iter_next(skiplist_iter_t *it);
slice_t skiplist_iter_key(const skiplist_iter_t *it);
slice_t skiplist_iter_value(const skiplist_iter_t *it);

#endif
