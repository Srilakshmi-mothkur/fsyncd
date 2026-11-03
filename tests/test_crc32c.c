#include <string.h>

#include "crc32c.h"
#include "test.h"

// the standard check values every CRC32C has to match
static void test_known_values(void) {
    const char *s = "123456789";
    CHECK(crc32c((const uint8_t *)s, strlen(s)) == 0xE3069283);

    uint8_t zeros[32] = {0};
    CHECK(crc32c(zeros, sizeof zeros) == 0x8A9136AA);

    CHECK(crc32c(NULL, 0) == 0);
}

static void test_extend(void) {
    const uint8_t *all = (const uint8_t *)"applered";
    uint32_t whole = crc32c(all, 8);
    uint32_t parts = crc32c_extend(crc32c(all, 5), all + 5, 3);
    CHECK(whole == parts);
}

// one flipped bit has to give a different checksum
static void test_one_bit_changes_it(void) {
    uint8_t data[] = { 'a', 'p', 'p', 'l', 'e' };
    uint32_t before = crc32c(data, sizeof data);
    data[2] ^= 0x01;
    CHECK(crc32c(data, sizeof data) != before);
}

// a cut off record has to give a different checksum
static void test_cut_off_changes_it(void) {
    const uint8_t *data = (const uint8_t *)"applered";
    CHECK(crc32c(data, 5) != crc32c(data, 8));
}

int main(void) {
    RUN_TEST(test_known_values);
    RUN_TEST(test_extend);
    RUN_TEST(test_one_bit_changes_it);
    RUN_TEST(test_cut_off_changes_it);
    return TEST_RESULT();
}
