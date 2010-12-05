#pragma once
#include "endianness.hpp"
#include "mm_base.hpp"
#include "../base/assert.hpp"
#include "../base/base.hpp"
#include "../std/memcpy.hpp"

#include "../base/start_mem_debug.hpp"

template <typename T, int Align = sizeof(T)> class MMVector
{
public:
  typedef T * const_iterator;
  typedef const_iterator iterator;
  typedef T value_type;

  const_iterator begin() const
  {
    return m_p;
  }

  const_iterator end() const
  {
    return m_p + m_Size;
  }

  T const & operator [] (size_t i) const
  {
    ASSERT_LESS(i, m_Size, ());
    return m_p[i];
  }

  size_t size() const
  {
    return m_Size;
  }

  void Parse(MMParseInfo & info)
  {
    if (!info.Successful()) return;
    uint32_t size;
    memcpy(size, info.Advance<uint8_t>(4), 4);
    Parse(info, SwapIfBigEndian(size));
  }

  void Parse(MMParseInfo & info, size_t vectorSize)
  {
    m_Size = vectorSize;
    if (!info.Successful()) return;
    m_p = info.Advance<T>(m_Size);
  }

private:
  T const * m_p;
  size_t m_Size;
};

#include "../base/stop_mem_debug.hpp"
