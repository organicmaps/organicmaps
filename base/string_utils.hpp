#pragma once

#include "../base/buffer_vector.hpp"

#include "../std/string.hpp"
#include "../std/stdint.hpp"
#include "../std/sstream.hpp"

#include "../3party/utfcpp/source/utf8/unchecked.h"

/// All methods work with strings in utf-8 format
namespace strings
{

typedef uint32_t UniChar;
typedef buffer_vector<UniChar, 32> UniString;

UniString MakeLowerCase(UniString const & s);
void MakeLowerCase(UniString & s);
void MakeLowerCase(string & s);
string MakeLowerCase(string const & s);
bool EqualNoCase(string const & s1, string const & s2);

template <typename DelimFuncT>
class TokenizeIterator
{
  typedef utf8::unchecked::iterator<string::const_iterator> Utf8IterT;
  Utf8IterT m_beg, m_end, m_finish;
  DelimFuncT m_delimFunc;

  /// Explicitly disabled, because we're storing iterators for string
  TokenizeIterator(char const *, DelimFuncT);

  void move()
  {
    m_beg = m_end;
    while (m_beg != m_finish)
    {
      if (m_delimFunc(*m_beg))
        ++m_beg;
      else
        break;
    }
    m_end = m_beg;
    while (m_end != m_finish)
    {
      if (m_delimFunc(*m_end))
        break;
      else
        ++m_end;
    }
  }

public:
  TokenizeIterator(string const & s, DelimFuncT delimFunc)
  : m_beg(s.begin()), m_end(s.begin()), m_finish(s.end()), m_delimFunc(delimFunc)
  {
    move();
  }

  string operator*() const
  {
    ASSERT( m_beg != m_finish, ("dereferencing of empty iterator") );
    return string(m_beg.base(), m_end.base());
  }

  operator bool() const { return m_beg != m_finish; }

  TokenizeIterator & operator++()
  {
    move();
    return (*this);
  }

  bool IsLast() const
  {
    if (!*this)
      return false;
    TokenizeIterator<DelimFuncT> copy(*this);
    ++copy;
    return !copy;
  }

  UniString GetUniString() const
  {
    UniString result;
    Utf8IterT iter(m_beg);
    while (iter != m_end)
    {
      result.push_back(*iter);
      ++iter;
    }
    return result;
  }
};

class SimpleDelimiter
{
  UniString m_delims;
public:
  SimpleDelimiter(char const * delimChars);
  /// @return true if c is delimiter
  bool operator()(UniChar c) const;
};

typedef TokenizeIterator<SimpleDelimiter> SimpleTokenizer;

template <typename FunctorT>
void Tokenize(string const & str, char const * delims, FunctorT f)
{
  SimpleTokenizer iter(str, delims);
  while (iter)
  {
    f(*iter);
    ++iter;
  }
}

/// @return code of last symbol in string or 0 if s is empty
UniChar LastUniChar(string const & s);

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

}
