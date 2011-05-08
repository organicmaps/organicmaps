#pragma once

#include "varint.hpp"

#include "../base/string_utils.hpp"
#include "../base/assert.hpp"

#include "../std/string.hpp"


class StringNumericOptimal
{
  string m_s;

  static const uint8_t numeric_bit = 1;

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
    m_s = utils::to_string(s);
    CHECK ( !m_s.empty(), () );
  }

  inline void Clear() { m_s.clear(); }
  inline bool IsEmpty() const { return m_s.empty(); }
  inline string Get() const { return m_s; }

  template <class TSink> void Write(TSink & sink) const
  {
    int n;
    if (utils::to_int(m_s, n) && n >= 0)
      WriteVarUint(sink, static_cast<uint32_t>((n << 1) | numeric_bit));
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
    uint32_t sz = ReadVarUint<uint32_t>(src);

    if ((sz & numeric_bit) != 0)
      m_s = utils::to_string(sz >> 1);
    else
    {
      sz = (sz >> 1) + 1;
      m_s.resize(sz);
      src.Read(&m_s[0], sz);
    }
  }
};
