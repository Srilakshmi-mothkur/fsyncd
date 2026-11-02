#include "coding.h"

void put_fixed32(uint8_t *out, uint32_t v) {
    out[0] = (uint8_t)v;
    out[1] = (uint8_t)(v >> 8);
    out[2] = (uint8_t)(v >> 16);
    out[3] = (uint8_t)(v >> 24);
}

void put_fixed64(uint8_t *out, uint64_t v) {
    put_fixed32(out, (uint32_t)v);
    put_fixed32(out + 4, (uint32_t)(v >> 32));
}

uint32_t get_fixed32(const uint8_t *in) {
    return (uint32_t)in[0] | ((uint32_t)in[1] << 8) |
           ((uint32_t)in[2] << 16) | ((uint32_t)in[3] << 24);
}

uint64_t get_fixed64(const uint8_t *in) {
    return (uint64_t)get_fixed32(in) | ((uint64_t)get_fixed32(in + 4) << 32);
}

size_t put_varint64(uint8_t *out, uint64_t v) {
    size_t i = 0;
    // For 300, the first loop writes 44 with the more flag (0x80) and leaves 2.
    // 2 is under 128, so the loop ends and 2 is written as the last byte.
    while (v >= 128) {
        out[i++] = (uint8_t)(v | 0x80);
        v >>= 7;   // same as v / 128
    }
    out[i++] = (uint8_t)v;
    return i;
}

size_t put_varint32(uint8_t *out, uint32_t v) {
    return put_varint64(out, v);
}

size_t get_varint64(const uint8_t *in, size_t len, uint64_t *v) {
    uint64_t result = 0;
    for (size_t i = 0; i < len && i < MAX_VARINT64_LEN; i++) {
        uint64_t b = in[i];
        // Bytes 1 to 9 carry 9 x 7 = 63 bits, so the 10th byte (i == 9) has only
        // 1 bit left to carry and can only be 0 or 1. More would not fit in 64 bits.
        if (i == 9 && b > 1) return 0;
        // b & 0x7F drops the more flag and keeps the piece. The piece is then
        // multiplied by 1, 128, 128 x 128 ... for its place and added.
        // For [44 more] [2 last], result = 44 x 1 + 2 x 128 = 300.
        result |= (b & 0x7F) << (7 * i);
        if ((b & 0x80) == 0) {   // no more flag, this was the last byte
            *v = result;
            return i + 1;
        }
    }
    return 0;
}

size_t get_varint32(const uint8_t *in, size_t len, uint32_t *v) {
    uint32_t result = 0;
    for (size_t i = 0; i < len && i < MAX_VARINT32_LEN; i++) {
        uint32_t b = in[i];
        // Bytes 1 to 4 carry 4 x 7 = 28 bits, so the 5th byte (i == 4) has only
        // 4 bits left to carry and can only be 0 to 15. More would not fit in 32 bits.
        if (i == 4 && b > 0x0F) return 0;
        result |= (b & 0x7F) << (7 * i);
        if ((b & 0x80) == 0) {
            *v = result;
            return i + 1;
        }
    }
    return 0;
}
