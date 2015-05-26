#pragma once

#include "coding/varint.hpp"

#include "base/string_utils.hpp"
#include "base/assert.hpp"

#include "std/limits.hpp"
#include "std/string.hpp"


class StringNumericOptimal
{
  string m_s;

public:
  inline bool operator== (StringNumericOptimal const & rhs) const
  {
    return m_s == rhs.m_s;
  }

  inline void Set(string const & s)
  {
    CHECK ( !s.empty(), () );
    m_s = s;
  }
  inline void Set(char const * p)
  {
    m_s = p;
    CHECK ( !m_s.empty(), () );
  }
  template <class T> void Set(T const & s)
  {
    m_s = strings::to_string(s);
    CHECK ( !m_s.empty(), () );
  }

  inline void Clear() { m_s.clear(); }
  inline bool IsEmpty() const { return m_s.empty(); }
  inline string const & Get() const { return m_s; }

  /// First uint64_t value is:
  /// - a number if low control bit is 1;
  /// - a string size-1 if low control bit is 0;

  template <class TSink> void Write(TSink & sink) const
  {
    // If string is a number and we have space for control bit
    uint64_t n;
    if (strings::to_uint64(m_s, n) && ((n << 1) >> 1) == n)
      WriteVarUint(sink, ((n << 1) | 1));
    else
    {
      size_t const sz = m_s.size();
      ASSERT_GREATER ( sz, 0, () );

      WriteVarUint(sink, static_cast<uint32_t>((sz-1) << 1));
      sink.Write(m_s.c_str(), sz);
    }
  }

  template <class TSource> void Read(TSource & src)
  {
    uint64_t sz = ReadVarUint<uint64_t>(src);

    if ((sz & 1) != 0)
      m_s = strings::to_string(sz >> 1);
    else
    {
      sz = (sz >> 1) + 1;
      ASSERT_LESS_OR_EQUAL(sz, numeric_limits<size_t>::max(), ("sz is out of platform's range."));
      m_s.resize(static_cast<size_t>(sz));
      src.Read(&m_s[0], sz);
    }
  }
};

inline string DebugPrint(StringNumericOptimal const & s)
{
  return s.Get();
}
