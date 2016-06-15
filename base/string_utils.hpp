#pragma once

#include "base/buffer_vector.hpp"

#include "std/algorithm.hpp"
#include "std/cstdint.hpp"
#include "std/iterator.hpp"
#include "std/limits.hpp"
#include "std/regex.hpp"
#include "std/sstream.hpp"
#include "std/string.hpp"

#include "3party/utfcpp/source/utf8/unchecked.h"

/// All methods work with strings in utf-8 format
namespace strings
{
typedef uint32_t UniChar;
// typedef buffer_vector<UniChar, 32> UniString;

/// Make new type, not typedef. Need to specialize DebugPrint.
class UniString : public buffer_vector<UniChar, 32>
{
  typedef buffer_vector<UniChar, 32> BaseT;

public:
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
/// For implementation @see base/normilize_unicode.cpp
void NormalizeInplace(UniString & s);
UniString Normalize(UniString const & s);

/// Replaces "full width" unicode digits with ascii ones.
void NormalizeDigits(string & utf8);
void NormalizeDigits(UniString & us);

/// Counts number of start symbols in string s (that is not lower and not normalized) that maches
/// to lower and normalized string low_s. If s doen't starts with low_s then returns 0; otherwise
/// returns number of start symbols in s that equivalent to lowStr
/// For implementation @see base/lower_case.cpp
size_t CountNormLowerSymbols(UniString const & s, UniString const & lowStr);

void AsciiToLower(string & s);
// TODO(AlexZ): current boost impl uses default std::locale() to trim.
// In general, it does not work for any unicode whitespace except ASCII U+0020 one.
void Trim(string & s);
/// Remove any characters that contain in "anyOf" on left and right side of string s
void Trim(string & s, char const * anyOf);

void MakeLowerCaseInplace(string & s);
string MakeLowerCase(string const & s);
bool EqualNoCase(string const & s1, string const & s2);

UniString MakeUniString(string const & utf8s);
string ToUtf8(UniString const & s);
bool IsASCIIString(string const & str);
bool IsASCIIDigit(UniChar c);
bool IsASCIILatin(UniChar c);

inline string DebugPrint(UniString const & s) { return ToUtf8(s); }

template <typename TDelimFn, typename TIt = UniString::const_iterator, bool KeepEmptyTokens = false>
class TokenizeIterator
{
public:
  using difference_type = std::ptrdiff_t;
  using value_type = string;
  using pointer = void;
  using reference = string;
  using iterator_category = std::input_iterator_tag;

  // *NOTE* |s| must be not temporary!
  TokenizeIterator(string const & s, TDelimFn const & delimFn)
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

  // Use default-constructed iterator for operator == to determine the
  // end of the token stream.
  TokenizeIterator() = default;

  string operator*() const
  {
    ASSERT(m_start != m_finish, ("Dereferencing of empty iterator."));
    return string(m_start.base(), m_end.base());
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
  using value_type = string;
  using pointer = void;
  using reference = string;
  using iterator_category = std::input_iterator_tag;

  // *NOTE* |s| must be not temporary!
  TokenizeIterator(string const & s, TDelimFn const & delimFn)
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

  // Use default-constructed iterator for operator == to determine the
  // end of the token stream.
  TokenizeIterator() = default;

  string operator*() const
  {
    ASSERT(!m_finished, ("Dereferencing of empty iterator."));
    return string(m_start.base(), m_end.base());
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

  // Used in TokenizeIterator to allow past the end iterator construction.
  SimpleDelimiter() = default;
  /// @return true if c is delimiter
  bool operator()(UniChar c) const;
};

using SimpleTokenizer =
    TokenizeIterator<SimpleDelimiter, ::utf8::unchecked::iterator<string::const_iterator>,
                     false /* KeepEmptyTokens */>;
using SimpleTokenizerWithEmptyTokens =
    TokenizeIterator<SimpleDelimiter, ::utf8::unchecked::iterator<string::const_iterator>,
                     true /* KeepEmptyTokens */>;

template <typename TFunctor>
void Tokenize(string const & str, char const * delims, TFunctor && f)
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
bool to_int(char const * s, int & i, int base = 10);
bool to_uint(char const * s, unsigned int & i, int base = 10);
bool to_uint64(char const * s, uint64_t & i);
bool to_int64(char const * s, int64_t & i);
bool to_double(char const * s, double & d);

inline bool is_number(string const & s)
{
  int64_t dummy;
  return to_int64(s.c_str(), dummy);
}

inline bool to_int(string const & s, int & i, int base = 10) { return to_int(s.c_str(), i, base); }
inline bool to_uint(string const & s, unsigned int & i, int base = 10)
{
  return to_uint(s.c_str(), i, base);
}
inline bool to_uint64(string const & s, uint64_t & i) { return to_uint64(s.c_str(), i); }
inline bool to_int64(string const & s, int64_t & i) { return to_int64(s.c_str(), i); }
inline bool to_double(string const & s, double & d) { return to_double(s.c_str(), d); }
//@}

/// @name From numeric to string.
//@{
inline string to_string(string const & s) { return s; }
inline string to_string(char const * s) { return s; }
template <typename T>
string to_string(T t)
{
  ostringstream ss;
  ss << t;
  return ss.str();
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

  return numeric_limits<T>::digits10 + is_signed<T>::value + 1;
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
string to_string_signed(T i)
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
  return string(beg, end - beg);
}

template <typename T>
string to_string_unsigned(T i)
{
  int const sz = UpperBoundOnChars<T>();
  char buf[sz];
  char * end = buf + sz;
  char * beg = to_string_digits(end, i);
  return string(beg, end - beg);
}
}

inline string to_string(int64_t i) { return impl::to_string_signed(i); }
inline string to_string(uint64_t i) { return impl::to_string_unsigned(i); }
/// Use this function to get string with fixed count of
/// "Digits after comma".
string to_string_dac(double d, int dac);
//@}

bool StartsWith(UniString const & s, UniString const & p);

bool StartsWith(string const & s1, char const * s2);

bool EndsWith(string const & s1, char const * s2);

bool EndsWith(string const & s1, string const & s2);

/// Try to guess if it's HTML or not. No guarantee.
bool IsHTML(string const & utf8);

/// Compare str1 and str2 and return if they are equal except for mismatchedSymbolsNum symbols
bool AlmostEqual(string const & str1, string const & str2, size_t mismatchedCount);

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
void ForEachMatched(string const & s, regex const & regex, TFn && fn)
{
  for (sregex_token_iterator cur(s.begin(), s.end(), regex), end; cur != end; ++cur)
    fn(*cur);
}

// Computes the minimum number of insertions, deletions and alterations
// of characters needed to transform one string into another.
// The function works in O(length1 * length2) time and memory
// where length1 and length2 are the lengths of the argument strings.
// See https://en.wikipedia.org/wiki/Levenshtein_distance.
// One string is [b1, e1) and the other is [b2, e2). The iterator
// form is chosen to fit both std::string and strings::UniString.
// This function does not normalize either of the strings.
template <typename TIter>
size_t EditDistance(TIter const & b1, TIter const & e1, TIter const & b2, TIter const & e2)
{
  size_t const n = distance(b1, e1);
  size_t const m = distance(b2, e2);
  // |cur| and |prev| are current and previous rows of the
  // dynamic programming table.
  vector<size_t> prev(m + 1);
  vector<size_t> cur(m + 1);
  for (size_t j = 0; j <= m; j++)
    prev[j] = j;
  auto it1 = b1;
  // 1-based to avoid corner cases.
  for (size_t i = 1; i <= n; ++i, ++it1)
  {
    cur[0] = i;
    auto const & c1 = *it1;
    auto it2 = b2;
    for (size_t j = 1; j <= m; ++j, ++it2)
    {
      auto const & c2 = *it2;

      cur[j] = min(cur[j - 1], prev[j]) + 1;
      cur[j] = min(cur[j], prev[j - 1] + (c1 == c2 ? 0 : 1));
    }
    prev.swap(cur);
  }
  return prev[m];
}
}  // namespace strings
