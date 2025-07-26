#include "coding/hex.hpp"

#include "base/assert.hpp"

namespace impl
{
static char const kToHexTable[] = "0123456789ABCDEF";

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

uint8_t HexDigitToRaw(uint8_t const digit)
{
  if (digit >= '0' && digit <= '9')
    return (digit - '0');
  else if (digit >= 'A' && digit <= 'F')
    return (digit - 'A' + 10);
  else if (digit >= 'a' && digit <= 'f')
    return (digit - 'a' + 10);
  ASSERT(false, (digit));
  return 0;
}

void FromHexRaw(void const * src, size_t size, void * dst)
{
  uint8_t const * ptr = static_cast<uint8_t const *>(src);
  uint8_t const * end = ptr + size;
  uint8_t * out = static_cast<uint8_t *>(dst);

  while (ptr < end)
  {
    *out = HexDigitToRaw(*ptr++) << 4;
    *out |= HexDigitToRaw(*ptr++);
    ++out;
  }
}
}  // namespace impl
