#ifndef FSYNCD_TEST_H
#define FSYNCD_TEST_H

// Small test helpers, see tests/test_version.c for usage.

#include <stdio.h>

static int test_failures = 0;

// prints the failed condition and keeps going
#define CHECK(cond)                                                    \
    do {                                                               \
        if (!(cond)) {                                                 \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n",               \
                    __FILE__, __LINE__, #cond);                        \
            test_failures++;                                           \
        }                                                              \
    } while (0)

// runs a test and prints ok or FAIL
#define RUN_TEST(fn)                                                   \
    do {                                                               \
        int failures_before = test_failures;                           \
        fn();                                                          \
        printf("%s %s\n",                                              \
               test_failures == failures_before ? "ok  " : "FAIL", #fn); \
    } while (0)

// return value for main
#define TEST_RESULT() (test_failures == 0 ? 0 : 1)

#endif
