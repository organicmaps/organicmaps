#pragma once

#include "base/buffer_vector.hpp"
#include "base/macros.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <limits>
#include <regex>
#include <sstream>
#include <string>
#include <type_traits>

#include "3party/utfcpp/source/utf8/unchecked.h"

/// All methods work with strings in utf-8 format
namespace strings
{
using UniChar = uint32_t;
// typedef buffer_vector<UniChar, 32> UniString;

/// Make new type, not typedef. Need to specialize DebugPrint.
class UniString : public buffer_vector<UniChar, 32>
{
  using BaseT = buffer_vector<UniChar, 32>;

public:
  using value_type = UniChar;

  UniString() {}
  explicit UniString(size_t n, UniChar c = UniChar()) : BaseT(n, c) {}

  template <class IterT>
  UniString(IterT b, IterT e)
    : BaseT(b, e)
  {
  }

  bool IsEqualAscii(char const * s) const;

  UniString & operator+=(UniString const & rhs)
  {
    append(rhs);
    return *this;
  }

  UniString operator+(UniString const & rhs) const
  {
    UniString result(*this);
    result += rhs;
    return result;
  }
};

/// Performs full case folding for string to make it search-compatible according
/// to rules in ftp://ftp.unicode.org/Public/UNIDATA/CaseFolding.txt
/// For implementation @see base/lower_case.cpp
void MakeLowerCaseInplace(UniString & s);
UniString MakeLowerCase(UniString const & s);

/// Performs NFKD - Compatibility decomposition for Unicode according
/// to rules in ftp://ftp.unicode.org/Public/UNIDATA/UnicodeData.txt
/// For implementation @see base/normalize_unicode.cpp
void NormalizeInplace(UniString & s);

UniString Normalize(UniString const & s);
std::string Normalize(std::string const & s);

/// Replaces "full width" unicode digits with ascii ones.
void NormalizeDigits(std::string & utf8);
void NormalizeDigits(UniString & us);

/// Counts number of start symbols in string s (that is not lower and not normalized) that maches
/// to lower and normalized string low_s. If s doen't starts with low_s then returns 0; otherwise
/// returns number of start symbols in s that equivalent to lowStr
/// For implementation @see base/lower_case.cpp
size_t CountNormLowerSymbols(UniString const & s, UniString const & lowStr);

void AsciiToLower(std::string & s);
// TODO(AlexZ): current boost impl uses default std::locale() to trim.
// In general, it does not work for any unicode whitespace except ASCII U+0020 one.
void Trim(std::string & s);
/// Remove any characters that contain in "anyOf" on left and right side of string s
void Trim(std::string & s, char const * anyOf);

// Replace the first match of the search substring in the input with the format string.
// str - An input string
// from - A substring to be searched for
// to - A substitute string
bool ReplaceFirst(std::string & str, std::string const & from, std::string const & to);

void MakeLowerCaseInplace(std::string & s);
std::string MakeLowerCase(std::string const & s);
bool EqualNoCase(std::string const & s1, std::string const & s2);

UniString MakeUniString(std::string const & utf8s);
std::string ToUtf8(UniString const & s);
bool IsASCIIString(std::string const & str);
bool IsASCIIDigit(UniChar c);
bool IsASCIISpace(UniChar c);
bool IsASCIILatin(UniChar c);

inline std::string DebugPrint(UniString const & s) { return ToUtf8(s); }

template <typename TDelimFn, typename TIt = UniString::const_iterator, bool KeepEmptyTokens = false>
class TokenizeIterator
{
public:
  using difference_type = std::ptrdiff_t;
  using value_type = std::string;
  using pointer = void;
  using reference = std::string;
  using iterator_category = std::input_iterator_tag;

  // *NOTE* |s| must be not temporary!
  TokenizeIterator(std::string const & s, TDelimFn const & delimFn)
    : m_start(s.begin()), m_end(s.begin()), m_finish(s.end()), m_delimFn(delimFn)
  {
    Move();
  }

  // *NOTE* |s| must be not temporary!
  TokenizeIterator(UniString const & s, TDelimFn const & delimFn)
    : m_start(s.begin()), m_end(s.begin()), m_finish(s.end()), m_delimFn(delimFn)
  {
    Move();
  }

  std::string operator*() const
  {
    ASSERT(m_start != m_finish, ("Dereferencing of empty iterator."));
    return std::string(m_start.base(), m_end.base());
  }

  UniString GetUniString() const
  {
    ASSERT(m_start != m_finish, ("Dereferencing of empty iterator."));
    return UniString(m_start, m_end);
  }

  operator bool() const { return m_start != m_finish; }

  TokenizeIterator & operator++()
  {
    Move();
    return *this;
  }

  bool operator==(TokenizeIterator const & rhs) const
  {
    if (!*this && !rhs)
      return true;
    if (*this && rhs)
      return m_start == rhs.m_start && m_end == rhs.m_end && m_finish == rhs.m_finish;
    return false;
  }

  bool operator!=(TokenizeIterator const & rhs) const { return !(*this == rhs); }

private:
  void Move()
  {
    m_start = m_end;
    while (m_start != m_finish && m_delimFn(*m_start))
      ++m_start;

    m_end = m_start;
    while (m_end != m_finish && !m_delimFn(*m_end))
      ++m_end;
  }

  // Token is defined as a pair (|m_start|, |m_end|), where:
  //
  // * m_start < m_end
  // * m_start == begin or m_delimFn(m_start - 1)
  // * m_end == m_finish or m_delimFn(m_end)
  // * for all i from [m_start, m_end): !m_delimFn(i)
  //
  // This version of TokenizeIterator iterates over all tokens and
  // keeps the invariant above.
  TIt m_start;
  TIt m_end;

  // The end of the string the iterator iterates over.
  TIt m_finish;

  TDelimFn m_delimFn;
};

template <typename TDelimFn, typename TIt>
class TokenizeIterator<TDelimFn, TIt, true /* KeepEmptyTokens */>
{
public:
  using difference_type = std::ptrdiff_t;
  using value_type = std::string;
  using pointer = void;
  using reference = std::string;
  using iterator_category = std::input_iterator_tag;

  // *NOTE* |s| must be not temporary!
  TokenizeIterator(std::string const & s, TDelimFn const & delimFn)
    : m_start(s.begin()), m_end(s.begin()), m_finish(s.end()), m_delimFn(delimFn), m_finished(false)
  {
    while (m_end != m_finish && !m_delimFn(*m_end))
      ++m_end;
  }

  // *NOTE* |s| must be not temporary!
  TokenizeIterator(UniString const & s, TDelimFn const & delimFn)
    : m_start(s.begin()), m_end(s.begin()), m_finish(s.end()), m_delimFn(delimFn), m_finished(false)
  {
    while (m_end != m_finish && !m_delimFn(*m_end))
      ++m_end;
  }

  std::string operator*() const
  {
    ASSERT(!m_finished, ("Dereferencing of empty iterator."));
    return std::string(m_start.base(), m_end.base());
  }

  UniString GetUniString() const
  {
    ASSERT(!m_finished, ("Dereferencing of empty iterator."));
    return UniString(m_start, m_end);
  }

  operator bool() const { return !m_finished; }

  TokenizeIterator & operator++()
  {
    Move();
    return *this;
  }

  bool operator==(TokenizeIterator const & rhs) const
  {
    if (!*this && !rhs)
      return true;
    if (*this && rhs)
    {
      return m_start == rhs.m_start && m_end == rhs.m_end && m_finish == rhs.m_finish &&
             m_finished == rhs.m_finished;
    }
    return false;
  }

  bool operator!=(TokenizeIterator const & rhs) const { return !(*this == rhs); }

private:
  void Move()
  {
    if (m_end == m_finish)
    {
      ASSERT(!m_finished, ());
      m_start = m_end = m_finish;
      m_finished = true;
      return;
    }

    m_start = m_end;
    ++m_start;

    m_end = m_start;
    while (m_end != m_finish && !m_delimFn(*m_end))
      ++m_end;
  }

  // Token is defined as a pair (|m_start|, |m_end|), where:
  //
  // * m_start <= m_end
  // * m_start == begin or m_delimFn(m_start - 1)
  // * m_end == m_finish or m_delimFn(m_end)
  // * for all i from [m_start, m_end): !m_delimFn(i)
  //
  // This version of TokenizeIterator iterates over all tokens and
  // keeps the invariant above.
  TIt m_start;
  TIt m_end;

  // The end of the string the iterator iterates over.
  TIt m_finish;

  TDelimFn m_delimFn;

  // When true, iterator is at the end position and is not valid
  // anymore.
  bool m_finished;
};

class SimpleDelimiter
{
  UniString m_delims;

public:
  SimpleDelimiter(char const * delims);

  SimpleDelimiter(char delim);

  // Returns true iff |c| is a delimiter.
  bool operator()(UniChar c) const;
};

using SimpleTokenizer =
    TokenizeIterator<SimpleDelimiter, ::utf8::unchecked::iterator<std::string::const_iterator>,
                     false /* KeepEmptyTokens */>;
using SimpleTokenizerWithEmptyTokens =
    TokenizeIterator<SimpleDelimiter, ::utf8::unchecked::iterator<std::string::const_iterator>,
                     true /* KeepEmptyTokens */>;

template <typename TFunctor>
void Tokenize(std::string const & str, char const * delims, TFunctor && f)
{
  SimpleTokenizer iter(str, delims);
  while (iter)
  {
    f(*iter);
    ++iter;
  }
}

template <template <typename ...> class Collection = std::vector>
Collection<std::string> Tokenize(std::string const & str, char const * delims)
{
  Collection<std::string> c;
  Tokenize(str, delims, base::MakeInsertFunctor(c));
  return c;
}

static_assert(std::is_same<std::vector<std::string>, decltype(strings::Tokenize("", ""))>::value,
              "Tokenize() should return vector<string> by default.");

/// Splits a string by the delimiter, keeps empty parts, on an empty string returns an empty vector.
/// Does not support quoted columns, newlines in columns and escaped quotes.
void ParseCSVRow(std::string const & s, char const delimiter, std::vector<std::string> & target);

/// @return code of last symbol in string or 0 if s is empty
UniChar LastUniChar(std::string const & s);

template <class T, size_t N, class TT>
bool IsInArray(T(&arr)[N], TT const & t)
{
  for (size_t i = 0; i < N; ++i)
    if (arr[i] == t)
      return true;
  return false;
}

/// @name From string to numeric.
//@{
WARN_UNUSED_RESULT bool to_int(char const * s, int & i, int base = 10);
WARN_UNUSED_RESULT bool to_uint(char const * s, unsigned int & i, int base = 10);
WARN_UNUSED_RESULT bool to_uint64(char const * s, uint64_t & i);
WARN_UNUSED_RESULT bool to_int64(char const * s, int64_t & i);
WARN_UNUSED_RESULT bool to_float(char const * s, float & f);
WARN_UNUSED_RESULT bool to_double(char const * s, double & d);

WARN_UNUSED_RESULT inline bool is_number(std::string const & s)
{
  int64_t dummy;
  return to_int64(s.c_str(), dummy);
}

WARN_UNUSED_RESULT inline bool to_int(std::string const & s, int & i, int base = 10) { return to_int(s.c_str(), i, base); }
WARN_UNUSED_RESULT inline bool to_uint(std::string const & s, unsigned int & i, int base = 10)
{
  return to_uint(s.c_str(), i, base);
}

WARN_UNUSED_RESULT inline bool to_uint64(std::string const & s, uint64_t & i) { return to_uint64(s.c_str(), i); }
WARN_UNUSED_RESULT inline bool to_int64(std::string const & s, int64_t & i) { return to_int64(s.c_str(), i); }
WARN_UNUSED_RESULT inline bool to_float(std::string const & s, float & f) { return to_float(s.c_str(), f); }
WARN_UNUSED_RESULT inline bool to_double(std::string const & s, double & d) { return to_double(s.c_str(), d); }
//@}

/// @name From numeric to string.
//@{
inline std::string to_string(std::string const & s) { return s; }
inline std::string to_string(char const * s) { return s; }
template <typename T>
std::string to_string(T t)
{
  std::ostringstream ss;
  ss << t;
  return ss.str();
}

WARN_UNUSED_RESULT inline bool to_any(std::string const & s, int & i) { return to_int(s, i); }
WARN_UNUSED_RESULT inline bool to_any(std::string const & s, unsigned int & i) { return to_uint(s, i); }
WARN_UNUSED_RESULT inline bool to_any(std::string const & s, uint64_t & i) { return to_uint64(s, i); }
WARN_UNUSED_RESULT inline bool to_any(std::string const & s, int64_t & i) { return to_int64(s, i); }
WARN_UNUSED_RESULT inline bool to_any(std::string const & s, float & f) { return to_float(s, f); }
WARN_UNUSED_RESULT inline bool to_any(std::string const & s, double & d) { return to_double(s, d); }
WARN_UNUSED_RESULT inline bool to_any(std::string const & s, std::string & result)
{
  result = s;
  return true;
}

namespace impl
{
template <typename T>
int UpperBoundOnChars()
{
  // It's wrong to return just numeric_limits<T>::digits10 + [is
  // signed] because digits10 for a type T is computed as:
  //
  // floor(log10(2 ^ (CHAR_BITS * sizeof(T)))) =
  // floor(CHAR_BITS * sizeof(T) * log10(2))
  //
  // Therefore, due to rounding, we need to compensate possible
  // error.
  //
  // NOTE: following code works only on two-complement systems!

  return std::numeric_limits<T>::digits10 + std::is_signed<T>::value + 1;
}

template <typename T>
char * to_string_digits(char * buf, T i)
{
  do
  {
    --buf;
    *buf = static_cast<char>(i % 10) + '0';
    i = i / 10;
  } while (i != 0);
  return buf;
}

template <typename T>
std::string to_string_signed(T i)
{
  bool const negative = i < 0;
  int const sz = UpperBoundOnChars<T>();
  char buf[sz];
  char * end = buf + sz;
  char * beg = to_string_digits(end, negative ? -i : i);
  if (negative)
  {
    --beg;
    *beg = '-';
  }
  return std::string(beg, end - beg);
}

template <typename T>
std::string to_string_unsigned(T i)
{
  int const sz = UpperBoundOnChars<T>();
  char buf[sz];
  char * end = buf + sz;
  char * beg = to_string_digits(end, i);
  return std::string(beg, end - beg);
}
}

inline std::string to_string(int32_t i) { return impl::to_string_signed(i); }
inline std::string to_string(int64_t i) { return impl::to_string_signed(i); }
inline std::string to_string(uint64_t i) { return impl::to_string_unsigned(i); }
/// Use this function to get string with fixed count of
/// "Digits after comma".
std::string to_string_dac(double d, int dac);
inline std::string to_string_with_digits_after_comma(double d, int dac) { return to_string_dac(d, dac); }
//@}

template <typename IterT1, typename IterT2>
bool StartsWith(IterT1 beg, IterT1 end, IterT2 begPrefix, IterT2 endPrefix)
{
  while (beg != end && begPrefix != endPrefix && *beg == *begPrefix)
  {
    ++beg;
    ++begPrefix;
  }
  return begPrefix == endPrefix;
}

bool StartsWith(UniString const & s, UniString const & p);

bool StartsWith(std::string const & s1, char const * s2);

bool StartsWith(std::string const & s1, std::string const & s2);

bool EndsWith(std::string const & s1, char const * s2);

bool EndsWith(std::string const & s1, std::string const & s2);

/// Try to guess if it's HTML or not. No guarantee.
bool IsHTML(std::string const & utf8);

/// Compare str1 and str2 and return if they are equal except for mismatchedSymbolsNum symbols
bool AlmostEqual(std::string const & str1, std::string const & str2, size_t mismatchedCount);

template <typename TIterator, typename TDelimiter>
typename TIterator::value_type JoinStrings(TIterator begin, TIterator end,
                                           TDelimiter const & delimiter)
{
  if (begin == end)
    return {};

  auto result = *begin++;
  for (TIterator it = begin; it != end; ++it)
  {
    result += delimiter;
    result += *it;
  }

  return result;
}

template <typename TContainer, typename TDelimiter>
typename TContainer::value_type JoinStrings(TContainer const & container,
                                            TDelimiter const & delimiter)
{
  return JoinStrings(container.begin(), container.end(), delimiter);
}

template <typename TFn>
void ForEachMatched(std::string const & s, std::regex const & regex, TFn && fn)
{
  for (std::sregex_token_iterator cur(s.begin(), s.end(), regex), end; cur != end; ++cur)
    fn(*cur);
}

// Computes the minimum number of insertions, deletions and
// alterations of characters needed to transform one string into
// another.  The function works in O(length1 * length2) time and
// O(min(length1, length2)) memory where length1 and length2 are the
// lengths of the argument strings.  See
// https://en.wikipedia.org/wiki/Levenshtein_distance.  One string is
// [b1, e1) and the other is [b2, e2). The iterator form is chosen to
// fit both std::string and strings::UniString.  This function does
// not normalize either of the strings.
template <typename TIter>
size_t EditDistance(TIter const & b1, TIter const & e1, TIter const & b2, TIter const & e2)
{
  size_t const n = std::distance(b1, e1);
  size_t const m = std::distance(b2, e2);

  if (m > n)
    return EditDistance(b2, e2, b1, e1);

  // |curr| and |prev| are current and previous rows of the
  // dynamic programming table.
  std::vector<size_t> prev(m + 1);
  std::vector<size_t> curr(m + 1);
  for (size_t j = 0; j <= m; ++j)
    prev[j] = j;
  auto it1 = b1;
  // 1-based to avoid corner cases.
  for (size_t i = 1; i <= n; ++i, ++it1)
  {
    curr[0] = i;
    auto const & c1 = *it1;
    auto it2 = b2;
    for (size_t j = 1; j <= m; ++j, ++it2)
    {
      auto const & c2 = *it2;

      curr[j] = std::min(curr[j - 1], prev[j]) + 1;
      curr[j] = std::min(curr[j], prev[j - 1] + (c1 == c2 ? 0 : 1));
    }
    prev.swap(curr);
  }
  return prev[m];
}
}  // namespace strings
