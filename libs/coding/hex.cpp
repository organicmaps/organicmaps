#include "coding/hex.hpp"

#include "base/assert.hpp"

namespace impl
{
static char const kToHexTable[] = "0123456789ABCDEF";

static uint8_t const kFromHexTable[] = {
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   255, 255, 255, 255, 255, 255, 255, 10,
    11,  12,  13,  14,  15,  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 10,  11,  12,  13,  14,  15,  255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};

void ToHexRaw(void const * src, size_t size, void * dst)
{
  uint8_t const * ptr = static_cast<uint8_t const *>(src);
  uint8_t const * end = ptr + size;
  uint8_t * out = static_cast<uint8_t *>(dst);

  while (ptr != end)
  {
    *out++ = kToHexTable[(*ptr) >> 4];
    *out++ = kToHexTable[(*ptr) & 0xF];
    ++ptr;
  }
}

bool FromHexRaw(void const * src, size_t size, void * dst)
{
  if (size % 2 != 0)
    return false;

  uint8_t const * ptr = static_cast<uint8_t const *>(src);
  uint8_t const * end = ptr + size;
  uint8_t * out = static_cast<uint8_t *>(dst);

  while (ptr < end)
  {
    uint8_t const d1 = kFromHexTable[*ptr++];
    uint8_t const d2 = kFromHexTable[*ptr++];
    if (d1 == 255 || d2 == 255)
      return false;

    *out++ = (d1 << 4) | d2;
  }
  return true;
}
}  // namespace impl
