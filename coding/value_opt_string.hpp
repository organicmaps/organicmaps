#pragma once

#include "coding/varint.hpp"

#include "base/string_utils.hpp"
#include "base/assert.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

/// If a string holds an uint value then stores it as a varint, otherwise as a string.
class StringNumericOptimal
{
public:
  bool operator==(StringNumericOptimal const & rhs) const
  {
    if (m_s != rhs.m_s)
    {
      uint64_t n1, n2;
      if (ToInt(n1) && rhs.ToInt(n2))
        return n1 == n2;
      return false;
    }
    return true;
  }

  void Set(std::string const & s)
  {
    CHECK(!s.empty(), ());
    m_s = s;
  }

  void Set(char const * p)
  {
    m_s = p;
    CHECK(!m_s.empty(), ());
  }

  template <typename T>
  void Set(T const & s)
  {
    m_s = strings::to_string(s);
    CHECK(!m_s.empty(), ());
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
    if (ToInt(n))
      WriteVarUint(sink, ((n << 1) | 1));
    else
    {
      size_t const sz = m_s.size();
      ASSERT_GREATER(sz, 0, ());

      WriteVarUint(sink, static_cast<uint32_t>((sz-1) << 1));
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
      ASSERT_LESS_OR_EQUAL(sz, std::numeric_limits<size_t>::max(),
                           ("sz is out of platform's range."));
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

inline std::string DebugPrint(StringNumericOptimal const & s) { return s.Get(); }
