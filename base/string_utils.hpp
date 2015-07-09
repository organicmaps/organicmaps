#pragma once

#include "base/buffer_vector.hpp"

#include "std/cstdint.hpp"
#include "std/limits.hpp"
#include "std/sstream.hpp"
#include "std/string.hpp"

#include "3party/utfcpp/source/utf8/unchecked.h"


/// All methods work with strings in utf-8 format
namespace strings
{

typedef uint32_t UniChar;
//typedef buffer_vector<UniChar, 32> UniString;

/// Make new type, not typedef. Need to specialize DebugPrint.
class UniString : public buffer_vector<UniChar, 32>
{
  typedef buffer_vector<UniChar, 32> BaseT;
public:
  UniString() {}
  explicit UniString(size_t n, UniChar c = UniChar()) : BaseT(n, c) {}
  template <class IterT> UniString(IterT b, IterT e) : BaseT(b, e) {}

  bool IsEqualAscii(char const * s) const;
};

/// Performs full case folding for string to make it search-compatible according
/// to rules in ftp://ftp.unicode.org/Public/UNIDATA/CaseFolding.txt
/// For implementation @see base/lower_case.cpp
void MakeLowerCaseInplace(UniString & s);
UniString MakeLowerCase(UniString const & s);

/// Performs NFKD - Compatibility decomposition for Unicode according
/// to rules in ftp://ftp.unicode.org/Public/UNIDATA/UnicodeData.txt
/// For implementation @see base/normilize_unicode.cpp
void NormalizeInplace(UniString & s);
UniString Normalize(UniString const & s);

/// Counts number of start symbols in string s (that is not lower and not normalized) that maches
/// to lower and normalized string low_s. If s doen't starts with low_s then returns 0; otherwise
/// returns number of start symbols in s that equivalent to lowStr
/// For implementation @see base/lower_case.cpp
size_t CountNormLowerSymbols(UniString const & s, UniString const & lowStr);

void AsciiToLower(string & s);
void Trim(string & s);

void MakeLowerCaseInplace(string & s);
string MakeLowerCase(string const & s);
bool EqualNoCase(string const & s1, string const & s2);

UniString MakeUniString(string const & utf8s);
string ToUtf8(UniString const & s);
bool IsASCIIString(string const & str);

inline string DebugPrint(UniString const & s)
{
  return ToUtf8(s);
}

template <typename DelimFuncT, typename UniCharIterT = UniString::const_iterator>
class TokenizeIterator
{
  UniCharIterT m_beg, m_end, m_finish;
  DelimFuncT m_delimFunc;

  /// Explicitly disabled, because we're storing iterators for string
  TokenizeIterator(char const *, DelimFuncT const &);

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
  /// @warning string S must be not temporary!
  TokenizeIterator(string const & s, DelimFuncT const & delimFunc)
  : m_beg(s.begin()), m_end(s.begin()), m_finish(s.end()), m_delimFunc(delimFunc)
  {
    move();
  }

  /// @warning unistring S must be not temporary!
  TokenizeIterator(UniString const & s, DelimFuncT const & delimFunc)
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

    TokenizeIterator<DelimFuncT, UniCharIterT> copy(*this);
    ++copy;
    return !copy;
  }

  UniString GetUniString() const
  {
    return UniString(m_beg, m_end);
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

typedef TokenizeIterator<SimpleDelimiter,
                         ::utf8::unchecked::iterator<string::const_iterator> > SimpleTokenizer;

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
    if (arr[i] == t)
      return true;
  return false;
}

/// @name From string to numeric.
//@{
bool to_int(char const * s, int & i);
bool to_uint64(char const * s, uint64_t & i);
bool to_int64(char const * s, int64_t & i);
bool to_double(char const * s, double & d);

inline bool is_number(string const & s) { int64_t dummy; return to_int64(s.c_str(), dummy); }

inline bool to_int(string const & s, int & i) { return to_int(s.c_str(), i); }
inline bool to_uint64(string const & s, uint64_t & i) { return to_uint64(s.c_str(), i); }
inline bool to_int64(string const & s, int64_t & i) { return to_int64(s.c_str(), i); }
inline bool to_double(string const & s, double & d) { return to_double(s.c_str(), d); }
//@}

/// @name From numeric to string.
//@{
inline string to_string(string const & s)
{
  return s;
}

inline string to_string(char const * s)
{
  return s;
}

template <typename T> string to_string(T t)
{
  ostringstream ss;
  ss << t;
  return ss.str();
}

namespace impl
{

template <typename T> char * to_string_digits(char * buf, T i)
{
  do
  {
    --buf;
    *buf = static_cast<char>(i % 10) + '0';
    i = i / 10;
  } while (i != 0);
  return buf;
}

template <typename T> string to_string_signed(T i)
{
  bool const negative = i < 0;
  int const sz = numeric_limits<T>::digits10 + 1;
  char buf[sz];
  char * end = buf + sz;
  char * beg = to_string_digits(end, negative ? -i : i);
  if (negative)
  {
    --beg;
    *beg = '-';
  }
  return string(beg, end - beg);
}

template <typename T> string to_string_unsigned(T i)
{
  int const sz = numeric_limits<T>::digits10;
  char buf[sz];
  char * end = buf + sz;
  char * beg = to_string_digits(end, i);
  return string(beg, end - beg);
}

}

inline string to_string(int64_t i)
{
  return impl::to_string_signed(i);
}

inline string to_string(uint64_t i)
{
  return impl::to_string_unsigned(i);
}

/// Use this function to get string with fixed count of
/// "Digits after comma".
string to_string_dac(double d, int dac);
//@}

bool StartsWith(string const & s1, char const * s2);

/// Try to guess if it's HTML or not. No guarantee.
bool IsHTML(string const & utf8);

/// Compare str1 and str2 and return if they are equal except for mismatchedSymbolsNum symbols
bool AlmostEqual(string const & str1, string const & str2, size_t mismatchedCount);

/*
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
*/
}
