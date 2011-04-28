#pragma once
#include "assert.hpp"
#include "base.hpp"
#include "../std/memcpy.hpp"

template <unsigned int kBytes> class Bitset
{
public:
  Bitset() { memset(&m_Data, 0, sizeof(m_Data)); }

  // Returns 1 if bit and 0 otherwise.
  uint8_t Bit(uint32_t offset) const
  {
    ASSERT_LESS(offset, kBytes, ());
    return (m_Data[offset >> 3] >> (offset & 7)) & 1;
  }

  void SetBit(uint32_t offset, bool bSet = true)
  {
    ASSERT_LESS(offset, kBytes, ());
    if (bSet)
      m_Data[offset >> 3] |= (1 << (offset & 7));
    else
      m_Data[offset >> 3] &= !(1 << (offset & 7));
  }

private:
  uint8_t m_Data[kBytes];
};
