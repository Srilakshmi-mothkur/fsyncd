#ifndef FSYNCD_CODING_H
#define FSYNCD_CODING_H

#include <stddef.h>
#include <stdint.h>

#define MAX_VARINT32_LEN 5
#define MAX_VARINT64_LEN 10

// Fixed numbers always take 4 or 8 bytes, smallest byte first, so files read
// the same on every machine. 0x12345678 is written as 78 56 34 12.
void put_fixed32(uint8_t *out, uint32_t v);
void put_fixed64(uint8_t *out, uint64_t v);
uint32_t get_fixed32(const uint8_t *in);
uint64_t get_fixed64(const uint8_t *in);

// A varint uses fewer bytes for small numbers. Each byte holds a piece of the
// number from 0 to 127, smallest piece first, and a flag saying if more bytes
// follow.
//   10  is written as [10 last]                 1 byte
//   300 is written as [44 more] [2 last]        2 bytes, 44 + 2 x 128 = 300
// Returns how many bytes were written.
size_t put_varint32(uint8_t *out, uint32_t v);
size_t put_varint64(uint8_t *out, uint64_t v);

// Reads one varint from the first len bytes of in. Returns how many bytes were
// read, or 0 if the data is broken, the bytes ran out while the last one still
// said more, or the number is too big to fit.
size_t get_varint32(const uint8_t *in, size_t len, uint32_t *v);
size_t get_varint64(const uint8_t *in, size_t len, uint64_t *v);

#endif
