#include <string.h>

#include "status.h"
#include "test.h"

static void test_ok_is_zero(void) {
    CHECK(ST_OK == 0);
}

static void test_status_names(void) {
    CHECK(strcmp(status_str(ST_OK), "ok") == 0);
    CHECK(strcmp(status_str(ST_NOT_FOUND), "not found") == 0);
    CHECK(strcmp(status_str(ST_CORRUPTION), "corruption") == 0);
    CHECK(strcmp(status_str(ST_IO_ERROR), "io error") == 0);
    CHECK(strcmp(status_str(ST_NO_MEMORY), "out of memory") == 0);
}

static void test_unknown_status(void) {
    CHECK(strcmp(status_str((status_t)99), "unknown") == 0);
}

int main(void) {
    RUN_TEST(test_ok_is_zero);
    RUN_TEST(test_status_names);
    RUN_TEST(test_unknown_status);
    return TEST_RESULT();
}
