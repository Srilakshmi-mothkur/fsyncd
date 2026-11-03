#include "crc32c.h"

uint32_t crc32c_extend(uint32_t crc, const uint8_t *data, size_t len) {
    // CRC32C starts from all bits set and flips them again at the end. Flipping
    // here also undoes the flip from the last call, so a checksum can be carried on.
    crc = ~crc;
    for (size_t i = 0; i < len; i++) {
        // Mix the next byte into the low 8 bits, then shift those 8 bits out one
        // at a time. Each time a 1 is shifted out, 0x82F63B78 is mixed in, which
        // spreads the byte across all 32 bits. That number is what makes it CRC32C.
        crc ^= data[i];
        for (int k = 0; k < 8; k++) {
            if (crc & 1) crc = (crc >> 1) ^ 0x82F63B78;
            else crc >>= 1;
        }
    }
    return ~crc;
}

uint32_t crc32c(const uint8_t *data, size_t len) {
    return crc32c_extend(0, data, len);
}
