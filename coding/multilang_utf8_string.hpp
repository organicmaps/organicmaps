#pragma once

#include "varint.hpp"

#include "../base/assert.hpp"

#include "../std/string.hpp"


class StringUtf8Multilang
{
  string m_s;

  size_t GetNextIndex(size_t i) const;
  char GetLangIndex(string const & lang) const;

public:

  void AddString(char lang, string const & utf8s);
  void AddString(string const & lang, string const & utf8s)
  {
    char const l = GetLangIndex(lang);
    if (l >= 0)
      AddString(l, utf8s);
  }

  bool GetString(char lang, string & utf8s) const;
  bool GetString(string const & lang, string & utf8s) const
  {
    char const l = GetLangIndex(lang);
    if (l >= 0)
      return GetString(l, utf8s);
    else
      return false;
  }

  template <class TSink> void Write(TSink & sink)
  {
    CHECK(!m_s.empty(), ());

    size_t const sz = m_s.size();
    WriteVarUint(sink, static_cast<uint32_t>(sz-1));
    sink.Write(m_s.c_str(), sz);
  }

  template <class TSource> void Read(TSource & src)
  {
    uint32_t const sz = ReadVarUint<uint32_t>(src) + 1;
    m_s.resize(sz);
    src.Read(&m_s[0], sz);

    CHECK(!m_s.empty(), ());
  }
};
