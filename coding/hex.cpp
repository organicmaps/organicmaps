#include "../base/SRC_FIRST.hpp"
#include "hex.hpp"


namespace impl {
  static const char kToHexTable[] = "0123456789ABCDEF";

  void ToHexRaw(void const * src, size_t size, void * dst)
  {
    uint8_t const * ptr = static_cast<uint8_t const *>(src);
    uint8_t const * end = ptr + size;
    uint8_t * out = static_cast<uint8_t*>(dst);

    while (ptr != end)
    {
      *out++ = kToHexTable[(*ptr) >> 4];
      *out++ = kToHexTable[(*ptr) & 0xF];
      ++ptr;
    }
  }

//  static const char kFromHexTable[] = "0123456789ABCDEF";

  uint8_t HexDigitToRaw(uint8_t const digit)
  {
    if (digit >= '0' && digit <= '9')
      return (digit - '0');
    else if (digit >= 'A' && digit <= 'F')
      return (digit - 'A' + 10);
    ASSERT(false, (digit));
    return 0;
  }

  void FromHexRaw(void const * src, size_t size, void * dst)
  {
    uint8_t const * ptr = static_cast<uint8_t const *>(src);
    uint8_t const * end = ptr + size;
    uint8_t * out = static_cast<uint8_t*>(dst);

    while (ptr < end)
    {
      *out = HexDigitToRaw(*ptr++) << 4;
      *out |= HexDigitToRaw(*ptr++);
      ++out;
    }
  }
}
