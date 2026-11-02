#include "coding.h"
#include "test.h"

static void test_fixed32(void) {
    uint8_t buf[4];
    put_fixed32(buf, 0x12345678);
    CHECK(buf[0] == 0x78 && buf[1] == 0x56 && buf[2] == 0x34 && buf[3] == 0x12);
    CHECK(get_fixed32(buf) == 0x12345678);
}

static void test_fixed64(void) {
    uint8_t buf[8];
    put_fixed64(buf, 0x0102030405060708ULL);
    CHECK(buf[0] == 0x08 && buf[7] == 0x01);
    CHECK(get_fixed64(buf) == 0x0102030405060708ULL);
}

// 300 is [44 more] [2 last]. 44 with the more flag is 44 + 128 = 172 = 0xAC.
static void test_varint_300(void) {
    uint8_t buf[MAX_VARINT32_LEN];
    CHECK(put_varint32(buf, 300) == 2);
    CHECK(buf[0] == 0xAC && buf[1] == 0x02);

    uint32_t v = 0;
    CHECK(get_varint32(buf, 2, &v) == 2);
    CHECK(v == 300);
}

// 127 is the biggest number that fits in 1 byte, 128 needs 2
static void test_varint_sizes(void) {
    uint64_t values[] = { 0, 1, 127, 128, 16383, 16384, UINT32_MAX, UINT64_MAX };
    size_t sizes[]    = { 1, 1, 1,   2,   2,     3,     5,          10 };
    for (size_t i = 0; i < sizeof values / sizeof values[0]; i++) {
        uint8_t buf[MAX_VARINT64_LEN];
        size_t n = put_varint64(buf, values[i]);
        CHECK(n == sizes[i]);

        uint64_t v = 0;
        CHECK(get_varint64(buf, n, &v) == n);
        CHECK(v == values[i]);
    }
}

// the biggest numbers end with 1 in byte 10 and 15 in byte 5
static void test_varint_last_byte(void) {
    uint8_t buf[MAX_VARINT64_LEN];
    CHECK(put_varint64(buf, UINT64_MAX) == 10);
    CHECK(buf[9] == 1);
    CHECK(put_varint32(buf, UINT32_MAX) == 5);
    CHECK(buf[4] == 15);
}

// last byte missing, so the data ends while still saying more
static void test_varint_cut_off(void) {
    uint8_t buf[MAX_VARINT64_LEN];
    size_t n = put_varint64(buf, UINT64_MAX);
    uint64_t v = 0;
    CHECK(get_varint64(buf, n - 1, &v) == 0);
}

// every byte says more, so it never ends
static void test_varint_never_ends(void) {
    uint8_t buf[11];
    for (int i = 0; i < 11; i++) buf[i] = 0xFF;
    uint64_t v = 0;
    CHECK(get_varint64(buf, sizeof buf, &v) == 0);
}

static void test_varint32_too_big(void) {
    uint8_t buf[MAX_VARINT64_LEN];
    size_t n = put_varint64(buf, (uint64_t)UINT32_MAX + 1);
    uint32_t v = 0;
    CHECK(get_varint32(buf, n, &v) == 0);
}

int main(void) {
    RUN_TEST(test_fixed32);
    RUN_TEST(test_fixed64);
    RUN_TEST(test_varint_300);
    RUN_TEST(test_varint_sizes);
    RUN_TEST(test_varint_last_byte);
    RUN_TEST(test_varint_cut_off);
    RUN_TEST(test_varint_never_ends);
    RUN_TEST(test_varint32_too_big);
    return TEST_RESULT();
}
