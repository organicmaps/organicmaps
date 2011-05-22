#pragma once

#include "../std/string.hpp"
#include "../std/stdint.hpp"

#include "../3party/utfcpp/source/utf8/unchecked.h"

namespace string_utils
{
  // get substrings from s divided by delim and pass them to toDo
  template <class ToDo> void TokenizeString(string const & s, char const * delim, ToDo toDo)
  {
    size_t const count = s.size();
    size_t i = 0;
    while (i < count)
    {
      i = s.find_first_not_of(delim, i);
      if (i == string::npos) return;

      size_t e = s.find_first_of(delim, i);
      if (e == string::npos) e = count;

      toDo(s.substr(i, e-i));

      i = e + 1;
    }
  }

  /// string tokenizer iterator
  class TokenizeIterator
  {
    size_t m_start, m_end;

    string const & m_src;
    char const * m_delim;

    void move();

  public:
    TokenizeIterator(string const & s, char const * delim);

    string operator*() const;

    TokenizeIterator & operator++();

    bool end() const { return (m_start >= m_end); }
    size_t is_last() const { return (m_end == m_src.size()); }
  };

  template <class T, size_t N, class TT> bool IsInArray(T (&arr) [N], TT const & t)
  {
    for (size_t i = 0; i < N; ++i)
      if (arr[i] == t) return true;
    return false;
  }

  bool to_int(char const * s, int & i);
  bool to_uint64(char const * s, uint64_t & i);
  bool to_int64(char const * s, int64_t & i);
  bool to_double(char const * s, double & d);

  template <class T>
  string to_string(T i)
  {
    ostringstream ss;
    ss << i;
    return ss.str();
  }

  inline bool to_int(string const & s, int & i) { return to_int(s.c_str(), i); }
  inline bool to_uint64(string const & s, uint64_t & i) { return to_uint64(s.c_str(), i); }
  inline bool to_int64(string const & s, int64_t & i) { return to_int64(s.c_str(), i); }
  inline bool to_double(string const & s, double & d) { return to_double(s.c_str(), d); }

  void make_lower_case(string & s);
  bool equal_no_case(string s1, string s2);

  inline string ToUtf8(wstring const & wstr)
  {
    string result;
    utf8::unchecked::utf16to8(wstr.begin(), wstr.end(), back_inserter(result));
    return result;
  }

  inline wstring FromUtf8(string const & str)
  {
    wstring result;
    utf8::unchecked::utf8to16(str.begin(), str.end(), back_inserter(result));
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
}
