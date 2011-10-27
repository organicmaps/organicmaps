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

  void FromHexRaw(void const * src, size_t size, void * dst)
  {
    uint8_t const * ptr = static_cast<uint8_t const *>(src);
    uint8_t const * end = ptr + size;
    uint8_t * out = static_cast<uint8_t*>(dst);

    while (ptr < end) {
      *out = 0;
      if (*ptr >= '0' && *ptr <= '9') {
        *out |= ((*ptr - '0') << 4);
      } else if (*ptr >= 'A' && *ptr <= 'F') {
        *out |= ((*ptr - 'A' + 10) << 4);
      }
      ++ptr;

      if (*ptr >= '0' && *ptr <= '9') {
        *out |= ((*ptr - '0') & 0xF);
      } else if (*ptr >= 'A' && *ptr <= 'F') {
        *out |= ((*ptr - 'A' + 10) & 0xF);
      }
      ++ptr;
      ++out;
    }
  }
}
