#include <stdio.h>
#include <string.h>

#include "skiplist.h"
#include "test.h"

static bool value_is(const skiplist_t *sl, const char *key, const char *want) {
    slice_t v;
    if (skiplist_get(sl, slice_from_str(key), &v) != ST_OK) return false;
    return slice_compare(v, slice_from_str(want)) == 0;
}

static void test_empty(void) {
    skiplist_t *sl = NULL;
    CHECK(skiplist_new(&sl) == ST_OK);
    slice_t v;
    CHECK(skiplist_get(sl, slice_from_str("apple"), &v) == ST_NOT_FOUND);
    CHECK(skiplist_count(sl) == 0);

    skiplist_iter_t it;
    skiplist_iter_init(&it, sl);
    CHECK(!skiplist_iter_valid(&it));
    skiplist_free(sl);
}

static void test_put_and_get(void) {
    skiplist_t *sl = NULL;
    CHECK(skiplist_new(&sl) == ST_OK);
    CHECK(skiplist_put(sl, slice_from_str("apple"), slice_from_str("red")) == ST_OK);
    CHECK(skiplist_put(sl, slice_from_str("kiwi"), slice_from_str("green")) == ST_OK);

    CHECK(value_is(sl, "apple", "red"));
    CHECK(value_is(sl, "kiwi", "green"));
    slice_t v;
    CHECK(skiplist_get(sl, slice_from_str("plum"), &v) == ST_NOT_FOUND);
    CHECK(skiplist_count(sl) == 2);
    CHECK(skiplist_bytes(sl) == 5 + 3 + 4 + 5);
    skiplist_free(sl);
}

static void test_replace(void) {
    skiplist_t *sl = NULL;
    CHECK(skiplist_new(&sl) == ST_OK);
    CHECK(skiplist_put(sl, slice_from_str("apple"), slice_from_str("red")) == ST_OK);
    CHECK(skiplist_put(sl, slice_from_str("apple"), slice_from_str("green")) == ST_OK);

    CHECK(value_is(sl, "apple", "green"));
    CHECK(skiplist_count(sl) == 1);
    CHECK(skiplist_bytes(sl) == 5 + 5);
    skiplist_free(sl);
}

// put in any order, the walk comes out sorted
static void test_iterate_sorted(void) {
    skiplist_t *sl = NULL;
    CHECK(skiplist_new(&sl) == ST_OK);
    const char *keys[] = { "melon", "apple", "plum", "cherry", "kiwi", "fig", "grape" };
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++) {
        CHECK(skiplist_put(sl, slice_from_str(keys[i]), slice_from_str("x")) == ST_OK);
    }

    const char *want[] = { "apple", "cherry", "fig", "grape", "kiwi", "melon", "plum" };
    size_t i = 0;
    skiplist_iter_t it;
    for (skiplist_iter_init(&it, sl); skiplist_iter_valid(&it); skiplist_iter_next(&it)) {
        CHECK(i < 7);
        if (i < 7) CHECK(slice_compare(skiplist_iter_key(&it), slice_from_str(want[i])) == 0);
        i++;
    }
    CHECK(i == 7);
    skiplist_free(sl);
}

static void test_many_keys(void) {
    skiplist_t *sl = NULL;
    CHECK(skiplist_new(&sl) == ST_OK);

    // key 0 to 9999 in a mixed up order, 7919 is prime so i * 7919 % 10000 hits each once
    char key[16], value[16];
    for (int i = 0; i < 10000; i++) {
        int n = (i * 7919) % 10000;
        snprintf(key, sizeof key, "key%05d", n);
        snprintf(value, sizeof value, "value%d", n);
        CHECK(skiplist_put(sl, slice_from_str(key), slice_from_str(value)) == ST_OK);
    }
    CHECK(skiplist_count(sl) == 10000);

    for (int n = 0; n < 10000; n++) {
        snprintf(key, sizeof key, "key%05d", n);
        snprintf(value, sizeof value, "value%d", n);
        CHECK(value_is(sl, key, value));
    }

    int expected = 0;
    skiplist_iter_t it;
    for (skiplist_iter_init(&it, sl); skiplist_iter_valid(&it); skiplist_iter_next(&it)) {
        snprintf(key, sizeof key, "key%05d", expected);
        CHECK(slice_compare(skiplist_iter_key(&it), slice_from_str(key)) == 0);
        expected++;
    }
    CHECK(expected == 10000);
    skiplist_free(sl);
}

static void test_empty_key_and_value(void) {
    skiplist_t *sl = NULL;
    CHECK(skiplist_new(&sl) == ST_OK);
    CHECK(skiplist_put(sl, slice_from_str(""), slice_from_str("")) == ST_OK);
    CHECK(skiplist_put(sl, slice_from_str("apple"), slice_from_str("red")) == ST_OK);
    CHECK(value_is(sl, "", ""));

    // the empty key sorts first
    skiplist_iter_t it;
    skiplist_iter_init(&it, sl);
    CHECK(skiplist_iter_valid(&it));
    CHECK(skiplist_iter_key(&it).len == 0);
    skiplist_free(sl);
}

// keys can hold a 0 byte, a C string would stop there
static void test_zero_byte_keys(void) {
    skiplist_t *sl = NULL;
    CHECK(skiplist_new(&sl) == ST_OK);
    uint8_t a[] = { 'a', 0, 'b' };
    uint8_t b[] = { 'a', 0, 'c' };
    slice_t ka = { a, 3 }, kb = { b, 3 };
    CHECK(skiplist_put(sl, kb, slice_from_str("second")) == ST_OK);
    CHECK(skiplist_put(sl, ka, slice_from_str("first")) == ST_OK);
    CHECK(skiplist_count(sl) == 2);

    skiplist_iter_t it;
    skiplist_iter_init(&it, sl);
    CHECK(slice_compare(skiplist_iter_key(&it), ka) == 0);
    skiplist_free(sl);
}

// the skiplist keeps its own copy, so the caller's bytes can change afterwards
static void test_copies_key_and_value(void) {
    skiplist_t *sl = NULL;
    CHECK(skiplist_new(&sl) == ST_OK);
    char key[] = "apple";
    char value[] = "red";
    CHECK(skiplist_put(sl, slice_from_str(key), slice_from_str(value)) == ST_OK);
    key[0] = 'X';
    value[0] = 'X';
    CHECK(value_is(sl, "apple", "red"));
    skiplist_free(sl);
}

int main(void) {
    RUN_TEST(test_empty);
    RUN_TEST(test_put_and_get);
    RUN_TEST(test_replace);
    RUN_TEST(test_iterate_sorted);
    RUN_TEST(test_many_keys);
    RUN_TEST(test_empty_key_and_value);
    RUN_TEST(test_zero_byte_keys);
    RUN_TEST(test_copies_key_and_value);
    return TEST_RESULT();
}
