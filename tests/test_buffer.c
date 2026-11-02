#include <string.h>

#include "buffer.h"
#include "test.h"

static void test_append(void) {
    buffer_t b;
    buffer_init(&b);
    CHECK(buffer_append(&b, "apple", 5) == ST_OK);
    CHECK(buffer_append(&b, "red", 3) == ST_OK);
    CHECK(b.len == 8);
    CHECK(memcmp(b.data, "applered", 8) == 0);
    buffer_destroy(&b);
}

static void test_grows(void) {
    buffer_t b;
    buffer_init(&b);
    uint8_t chunk[100] = {0};
    for (int i = 0; i < 50; i++) {
        CHECK(buffer_append(&b, chunk, sizeof chunk) == ST_OK);
    }
    CHECK(b.len == 5000);
    CHECK(b.cap >= 5000);
    buffer_destroy(&b);
}

static void test_clear_keeps_memory(void) {
    buffer_t b;
    buffer_init(&b);
    CHECK(buffer_append(&b, "apple", 5) == ST_OK);
    size_t cap = b.cap;
    buffer_clear(&b);
    CHECK(b.len == 0);
    CHECK(b.cap == cap);
    buffer_destroy(&b);
}

static void test_append_nothing(void) {
    buffer_t b;
    buffer_init(&b);
    CHECK(buffer_append(&b, NULL, 0) == ST_OK);
    CHECK(b.len == 0);
    buffer_destroy(&b);
}

static void test_slice(void) {
    buffer_t b;
    buffer_init(&b);
    CHECK(buffer_append(&b, "apple", 5) == ST_OK);
    slice_t s = buffer_slice(&b);
    CHECK(slice_compare(s, slice_from_str("apple")) == 0);
    buffer_destroy(&b);
}

int main(void) {
    RUN_TEST(test_append);
    RUN_TEST(test_grows);
    RUN_TEST(test_clear_keeps_memory);
    RUN_TEST(test_append_nothing);
    RUN_TEST(test_slice);
    return TEST_RESULT();
}
