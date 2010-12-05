#pragma once

#include "../base/base.hpp"

#include "../std/string.hpp"
#include "../std/memcpy.hpp"

#include "../3party/utfcpp/source/utf8.h"

#include "../base/start_mem_debug.hpp"

typedef basic_string<uint8_t> byte_string;

inline byte_string StringToByteString(string const & str)
{
  byte_string result;
  result.resize(str.size());
  memcpy(&result[0], &str[0], str.size());
  return result;
}

inline string ToUtf8(wstring const & wstr)
{
  string result;
  utf8::utf16to8(wstr.begin(), wstr.end(), back_inserter(result));
  return result;
}

inline wstring FromUtf8(string const & str)
{
  wstring result;
  utf8::utf8to16(str.begin(), str.end(), back_inserter(result));
  return result;
}

template <typename ItT, typename DelimiterT>
typename ItT::value_type JoinStrings(ItT begin, ItT end, DelimiterT const & delimiter)
{
  typedef typename ItT::value_type StringT;

  if (begin == end) return StringT();

  StringT result = *begin++;
  for (ItT it = begin; it != end; ++it)
  {
    result += delimiter;
    result += *it;
  }

  return result;
}

template <typename ContainerT, typename DelimiterT>
typename ContainerT::value_type JoinStrings(ContainerT const & container,
                                            DelimiterT const & delimiter)
{
  return JoinStrings(container.begin(), container.end(), delimiter);
}

inline bool IsPrefixOf(string const & s1, string const & s2)
{
  if (s1.size() > s2.size()) return false;

  for (size_t i = 0; i < s1.size(); ++i)
  {
    if (s1[i] != s2[i]) return false;
  }

  return true;
}

#include "../base/stop_mem_debug.hpp"
