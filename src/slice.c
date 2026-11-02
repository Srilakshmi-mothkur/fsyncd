#include <string.h>

#include "slice.h"

slice_t slice_from_str(const char *s) {
    slice_t sl = { (const uint8_t *)s, strlen(s) };
    return sl;
}

int slice_compare(slice_t a, slice_t b) {
    size_t n = a.len < b.len ? a.len : b.len;
    int r = memcmp(a.data, b.data, n);
    if (r != 0) return r;
    if (a.len < b.len) return -1;
    if (a.len > b.len) return 1;
    return 0;
}
