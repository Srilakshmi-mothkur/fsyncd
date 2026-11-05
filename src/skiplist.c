#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "skiplist.h"

// No key is taller than 20 levels, enough for about a million keys.
#define MAX_HEIGHT 20

struct skiplist_node {
    uint8_t *key;
    size_t key_len;
    uint8_t *value;
    size_t value_len;
    int height;                // Number of levels the node is on.
    skiplist_node_t *next[];   // next[i] is the next node on level i. A node 3 levels
                               // tall has next[0], next[1] and next[2].
};

struct skiplist {
    skiplist_node_t *head;     // Not a real key, every level starts from it.
    int height;                // Levels in use, searches start from the top one.
    uint64_t rng;              // Used for the coin flips.
    size_t count;
    size_t bytes;
};

static slice_t node_key(const skiplist_node_t *n) {
    slice_t s = { n->key, n->key_len };
    return s;
}

// One extra byte so an empty key still gets memory, then NULL only means
// malloc failed.
static uint8_t *copy_bytes(slice_t s) {
    uint8_t *p = malloc(s.len + 1);
    if (p != NULL && s.len > 0) memcpy(p, s.data, s.len);
    return p;
}

// Gives a new random looking number each call. It starts from the same number
// every time, so a test always makes the same coin flips and a failing test
// fails the same way again.
static uint64_t next_random(skiplist_t *sl) {
    uint64_t x = sl->rng;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    sl->rng = x;
    return x;
}

// Flips coins to pick how many levels a new key is on. Every key gets level 0,
// then each heads adds a level until the first tails. About half the keys get
// level 1, a quarter get level 2, and very few get tall.
static int random_height(skiplist_t *sl) {
    int h = 1;
    while (h < MAX_HEIGHT && (next_random(sl) & 1)) h++;
    return h;
}

// Returns the first node with a key >= key, or NULL if there is none.
// If prev is not NULL, prev[level] is set to the node just before where key
// belongs on each level. Put links the new node in right after those.
static skiplist_node_t *find_ge(const skiplist_t *sl, slice_t key, skiplist_node_t **prev) {
    skiplist_node_t *x = sl->head;
    for (int level = sl->height - 1; level >= 0; level--) {
        while (x->next[level] != NULL && slice_compare(node_key(x->next[level]), key) < 0) {
            x = x->next[level];
        }
        if (prev != NULL) prev[level] = x;
    }
    return x->next[0];
}

status_t skiplist_new(skiplist_t **out) {
    skiplist_t *sl = calloc(1, sizeof *sl);
    if (sl == NULL) return ST_NO_MEMORY;

    // calloc fills the memory with zeros, so all of head's next pointers start
    // as NULL and every level is empty.
    sl->head = calloc(1, sizeof *sl->head + MAX_HEIGHT * sizeof sl->head->next[0]);
    if (sl->head == NULL) {
        free(sl);
        return ST_NO_MEMORY;
    }
    sl->head->height = MAX_HEIGHT;
    sl->height = 1;
    sl->rng = 0x2545F4914F6CDD1DULL;
    *out = sl;
    return ST_OK;
}

// Walks level 0 and frees every node. Every node is on level 0 once. The higher
// levels point at the same nodes, so freeing through them would free a node twice.
void skiplist_free(skiplist_t *sl) {
    if (sl == NULL) return;
    skiplist_node_t *x = sl->head->next[0];
    while (x != NULL) {
        skiplist_node_t *next = x->next[0];
        free(x->key);
        free(x->value);
        free(x);
        x = next;
    }
    free(sl->head);
    free(sl);
}

// Gives an existing key a new value. Copies the new value before freeing the old
// one, so if the copy fails the node still has its old value.
static status_t replace_value(skiplist_t *sl, skiplist_node_t *n, slice_t value) {
    uint8_t *v = copy_bytes(value);
    if (v == NULL) return ST_NO_MEMORY;
    sl->bytes = sl->bytes - n->value_len + value.len;
    free(n->value);
    n->value = v;
    n->value_len = value.len;
    return ST_OK;
}

status_t skiplist_put(skiplist_t *sl, slice_t key, slice_t value) {
    // Find where key belongs. If it's already there, only the value changes.
    skiplist_node_t *prev[MAX_HEIGHT];
    skiplist_node_t *x = find_ge(sl, key, prev);
    if (x != NULL && slice_compare(node_key(x), key) == 0) {
        return replace_value(sl, x, value);
    }

    // Make the node and both copies first. If any malloc fails, free what we got
    // and return, the list stays as it was.
    int h = random_height(sl);
    skiplist_node_t *n = malloc(sizeof *n + (size_t)h * sizeof n->next[0]);
    uint8_t *k = copy_bytes(key);
    uint8_t *v = copy_bytes(value);
    if (n == NULL || k == NULL || v == NULL) {
        free(n);
        free(k);
        free(v);
        return ST_NO_MEMORY;
    }
    n->key = k;
    n->key_len = key.len;
    n->value = v;
    n->value_len = value.len;
    n->height = h;

    // If the new node is taller than every other node, its top levels are empty,
    // so it goes right after head there.
    if (h > sl->height) {
        for (int level = sl->height; level < h; level++) prev[level] = sl->head;
        sl->height = h;
    }

    // Link it in on each level. The new node points at what came after prev,
    // then prev points at the new node.
    for (int level = 0; level < h; level++) {
        n->next[level] = prev[level]->next[level];
        prev[level]->next[level] = n;
    }

    sl->count++;
    sl->bytes += key.len + value.len;
    return ST_OK;
}

// find_ge gives the first key >= key, so it's only a match if it is exactly key.
status_t skiplist_get(const skiplist_t *sl, slice_t key, slice_t *value) {
    skiplist_node_t *x = find_ge(sl, key, NULL);
    if (x == NULL || slice_compare(node_key(x), key) != 0) return ST_NOT_FOUND;
    value->data = x->value;
    value->len = x->value_len;
    return ST_OK;
}

size_t skiplist_count(const skiplist_t *sl) {
    return sl->count;
}

size_t skiplist_bytes(const skiplist_t *sl) {
    return sl->bytes;
}

// Level 0 has every key in order, so the walk only follows next[0].
void skiplist_iter_init(skiplist_iter_t *it, const skiplist_t *sl) {
    it->node = sl->head->next[0];
}

bool skiplist_iter_valid(const skiplist_iter_t *it) {
    return it->node != NULL;
}

void skiplist_iter_next(skiplist_iter_t *it) {
    it->node = it->node->next[0];
}

slice_t skiplist_iter_key(const skiplist_iter_t *it) {
    return node_key(it->node);
}

slice_t skiplist_iter_value(const skiplist_iter_t *it) {
    slice_t s = { it->node->value, it->node->value_len };
    return s;
}
