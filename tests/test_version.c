#include <string.h>

#include "fsyncd.h"
#include "test.h"

static void test_version_is_set(void) {
    const char *v = fsyncd_version();
    CHECK(v != NULL);
    CHECK(strcmp(v, "0.0.1") == 0);
}

int main(void) {
    RUN_TEST(test_version_is_set);
    return TEST_RESULT();
}
