#pragma once

#include "coding/varint.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

#include <limits>
#include <string>

class StringNumericOptimal
{
public:
  bool operator==(StringNumericOptimal const & rhs) const { return m_s == rhs.m_s; }

  void Set(std::string const & s)
  {
    CHECK(!s.empty(), ());
    m_s = s;
  }

  void Clear() { m_s.clear(); }
  bool IsEmpty() const { return m_s.empty(); }
  std::string const & Get() const { return m_s; }

  /// First uint64_t value is:
  /// - a number if low control bit is 1;
  /// - a string size-1 if low control bit is 0;
  template <typename Sink>
  void Write(Sink & sink) const
  {
    uint64_t n;
    if (ToInt(n) && (m_s.size() == 1 || m_s.front() != '0'))
      WriteVarUint(sink, ((n << 1) | 1));
    else
    {
      size_t const sz = m_s.size();
      ASSERT_GREATER(sz, 0, ());

      WriteVarUint(sink, static_cast<uint32_t>((sz - 1) << 1));
      sink.Write(m_s.c_str(), sz);
    }
  }

  template <typename Source>
  void Read(Source & src)
  {
    uint64_t sz = ReadVarUint<uint64_t>(src);

    if ((sz & 1) != 0)
      m_s = strings::to_string(sz >> 1);
    else
    {
      sz = (sz >> 1) + 1;
      ASSERT_LESS_OR_EQUAL(sz, std::numeric_limits<size_t>::max(), ("sz is out of platform's range."));
      m_s.resize(static_cast<size_t>(sz));
      src.Read(&m_s[0], static_cast<size_t>(sz));
    }
  }

private:
  bool ToInt(uint64_t & n) const
  {
    // If string is a number and we have space for control bit
    return (strings::to_uint64(m_s, n) && ((n << 1) >> 1) == n);
  }

  std::string m_s;
};

inline std::string DebugPrint(StringNumericOptimal const & s)
{
  return s.Get();
}
