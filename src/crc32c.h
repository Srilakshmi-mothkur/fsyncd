#ifndef FSYNCD_CRC32C_H
#define FSYNCD_CRC32C_H

#include <stddef.h>
#include <stdint.h>

// A checksum of data, used to find out if bytes on disk got broken. The same bytes
// always give the same number, and changing even one bit gives a different one.
uint32_t crc32c(const uint8_t *data, size_t len);

// Carries on a checksum with more bytes, for data that is not in one place.
// crc32c_extend(crc32c(a), b) is the same as crc32c of a and b joined together.
uint32_t crc32c_extend(uint32_t crc, const uint8_t *data, size_t len);

#endif
