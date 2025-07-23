#pragma once

#include "search/common.hpp"

#include "base/assert.hpp"
#include "base/range_iterator.hpp"

#include <cstdint>
#include <sstream>
#include <string>

namespace search
{
class TokenRange final
{
public:
  TokenRange() = default;
  TokenRange(size_t begin, size_t end) : m_begin(static_cast<uint8_t>(begin)), m_end(static_cast<uint8_t>(end))

  {
    ASSERT_LESS_OR_EQUAL(begin, kMaxNumTokens, ());
    ASSERT_LESS_OR_EQUAL(end, kMaxNumTokens, ());
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

  inline bool operator==(TokenRange const & rhs) const { return m_begin == rhs.m_begin && m_end == rhs.m_end; }

  inline size_t Begin() const { return m_begin; }
  inline size_t End() const { return m_end; }

  inline base::RangeIterator<size_t> begin() const { return base::RangeIterator<size_t>(m_begin); }
  inline base::RangeIterator<size_t> end() const { return base::RangeIterator<size_t>(m_end); }

  inline base::RangeIterator<size_t> cbegin() const { return base::RangeIterator<size_t>(m_begin); }
  inline base::RangeIterator<size_t> cend() const { return base::RangeIterator<size_t>(m_end); }

private:
  friend std::string DebugPrint(TokenRange const & tokenRange);

  uint8_t m_begin = 0;
  uint8_t m_end = 0;
};

inline std::string DebugPrint(TokenRange const & tokenRange)
{
  std::ostringstream os;
  os << "TokenRange [" << tokenRange.Begin() << ", " << tokenRange.End() << ")";
  return os.str();
}
}  // namespace search
