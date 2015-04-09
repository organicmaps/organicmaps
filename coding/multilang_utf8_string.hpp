#pragma once

#include "coding/varint.hpp"

#include "base/assert.hpp"

#include "std/string.hpp"


namespace utils
{
  template <class TSink> void WriteString(TSink & sink, string const & s)
  {
    CHECK(!s.empty(), ());

    size_t const sz = s.size();
    WriteVarUint(sink, static_cast<uint32_t>(sz-1));
    sink.Write(s.c_str(), sz);
  }

  template <class TSource> void ReadString(TSource & src, string & s)
  {
    uint32_t const sz = ReadVarUint<uint32_t>(src) + 1;
    s.resize(sz);
    src.Read(&s[0], sz);

    CHECK(!s.empty(), ());
  }
}

class StringUtf8Multilang
{
  string m_s;

  size_t GetNextIndex(size_t i) const;

public:
  static int8_t const UNSUPPORTED_LANGUAGE_CODE = -1;
  static int8_t const DEFAULT_CODE = 0;

  /// @return UNSUPPORTED_LANGUAGE_CODE if language is not recognized
  static int8_t GetLangIndex(string const & lang);
  /// @return empty string if langCode is invalid
  static char const * GetLangByCode(int8_t langCode);

  inline bool operator== (StringUtf8Multilang const & rhs) const
  {
    return (m_s == rhs.m_s);
  }

  inline void Clear() { m_s.clear(); }
  inline bool IsEmpty() const { return m_s.empty(); }

  void AddString(int8_t lang, string const & utf8s);
  void AddString(string const & lang, string const & utf8s)
  {
    int8_t const l = GetLangIndex(lang);
    if (l >= 0)
      AddString(l, utf8s);
  }

  template <class T>
  void ForEachRef(T & functor) const
  {
    size_t i = 0;
    size_t const sz = m_s.size();
    while (i < sz)
    {
      size_t const next = GetNextIndex(i);
      if (!functor((m_s[i] & 0x3F), m_s.substr(i + 1, next - i - 1)))
        return;
      i = next;
    }
  }

  bool GetString(int8_t lang, string & utf8s) const;
  bool GetString(string const & lang, string & utf8s) const
  {
    int8_t const l = GetLangIndex(lang);
    if (l >= 0)
      return GetString(l, utf8s);
    else
      return false;
  }

  int8_t FindString(string const & utf8s) const;

  template <class TSink> void Write(TSink & sink) const
  {
    utils::WriteString(sink, m_s);
  }

  template <class TSource> void Read(TSource & src)
  {
    utils::ReadString(src, m_s);
  }
};

string DebugPrint(StringUtf8Multilang const & s);
