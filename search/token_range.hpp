#pragma once

#include "base/assert.hpp"
#include "base/range_iterator.hpp"

#include <cstdint>
#include <sstream>
#include <string>

namespace search
{
struct TokenRange final
{
  TokenRange() = default;
  TokenRange(size_t begin, size_t end) : m_begin(begin), m_end(end)
  {
    ASSERT(IsValid(), (*this));
  }

  inline bool AdjacentTo(TokenRange const & rhs) const
  {
    ASSERT(IsValid(), (*this));
    ASSERT(rhs.IsValid(), (rhs));
    return m_begin == rhs.m_end || m_end == rhs.m_begin;
  }

  inline size_t Size() const
  {
    ASSERT(IsValid(), (*this));
    return m_end - m_begin;
  }

  inline bool Empty() const { return Size() == 0; }

  inline void Clear()
  {
    m_begin = 0;
    m_end = 0;
  }

  inline bool IsValid() const { return m_begin <= m_end; }

  inline bool operator<(TokenRange const & rhs) const
  {
    if (m_begin != rhs.m_begin)
      return m_begin < rhs.m_begin;
    return m_end < rhs.m_end;
  }

  inline bool operator==(TokenRange const & rhs) const
  {
    return m_begin == rhs.m_begin && m_end == rhs.m_end;
  }

  inline my::RangeIterator<size_t> begin() const { return my::RangeIterator<size_t>(m_begin); }
  inline my::RangeIterator<size_t> end() const { return my::RangeIterator<size_t>(m_end); }

  inline my::RangeIterator<size_t> cbegin() const { return my::RangeIterator<size_t>(m_begin); }
  inline my::RangeIterator<size_t> cend() const { return my::RangeIterator<size_t>(m_end); }

  size_t m_begin = 0;
  size_t m_end = 0;
};

inline std::string DebugPrint(TokenRange const & tokenRange)
{
  std::ostringstream os;
  os << "TokenRange [" << tokenRange.m_begin << ", " << tokenRange.m_end << ")";
  return os.str();
}
}  // namespace search
