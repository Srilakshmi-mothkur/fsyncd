#include "slice.h"
#include "test.h"

static void test_from_str(void) {
    slice_t s = slice_from_str("apple");
    CHECK(s.len == 5);
    CHECK(s.data[0] == 'a');
}

static void test_compare(void) {
    slice_t apple = slice_from_str("apple");
    slice_t apricot = slice_from_str("apricot");
    slice_t app = slice_from_str("app");

    CHECK(slice_compare(apple, apricot) < 0);
    CHECK(slice_compare(apricot, apple) > 0);
    CHECK(slice_compare(app, apple) < 0);
    CHECK(slice_compare(apple, slice_from_str("apple")) == 0);
}

static void test_compare_empty(void) {
    slice_t empty = slice_from_str("");
    CHECK(slice_compare(empty, empty) == 0);
    CHECK(slice_compare(empty, slice_from_str("a")) < 0);
}

// keys can hold a 0 byte, a C string would stop there
static void test_compare_zero_byte(void) {
    uint8_t one[] = { 'a', 0, 'b' };
    uint8_t two[] = { 'a', 0, 'c' };
    slice_t a = { one, 3 };
    slice_t b = { two, 3 };
    CHECK(slice_compare(a, b) < 0);
}

int main(void) {
    RUN_TEST(test_from_str);
    RUN_TEST(test_compare);
    RUN_TEST(test_compare_empty);
    RUN_TEST(test_compare_zero_byte);
    return TEST_RESULT();
}
