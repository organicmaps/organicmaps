#include "base/string_utils.hpp"

#include "base/assert.hpp"
#include "base/math.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iterator>

#include <fast_double_parser.h>
#include <boost/algorithm/string/trim.hpp>
#include <string>

namespace strings
{
namespace
{
template <typename T>
T RealConverter(char const * start, char ** stop);

template <>
float RealConverter<float>(char const * start, char ** stop)
{
  // . or , parsing depends on locale!
  return std::strtof(start, stop);
}

template <>
double RealConverter<double>(char const * start, char ** stop)
{
  // . or , parsing depends on locale!
  return std::strtod(start, stop);
}

template <typename T>
bool ToReal(char const * start, T & result)
{
  // Try faster implementation first.
  double d;
  // TODO(AB): replace with more robust dependency that doesn't use std::is_finite in the implementation.
  char const * endptr = fast_double_parser::parse_number(start, &d);
  if (endptr == nullptr)
  {
    // Fallback to our implementation, it supports numbers like "1."
    char * stop;
    result = RealConverter<T>(start, &stop);
    if (*stop == 0 && start != stop && math::is_finite(result))
      return true;
  }
  else if (*endptr == 0 && math::is_finite(d))
  {
    result = static_cast<T>(d);
    return true;
  }
  // Do not parse strings that contain additional non-number characters.
  return false;
}

}  // namespace

UniString UniString::kSpace = MakeUniString(" ");

bool UniString::IsEqualAscii(char const * s) const
{
  return (size() == strlen(s) && std::equal(begin(), end(), s));
}

SimpleDelimiter::SimpleDelimiter(char const * delims)
{
  std::string_view const s(delims);
  auto it = s.begin();
  while (it != s.end())
    m_delims.push_back(utf8::unchecked::next(it));
}

SimpleDelimiter::SimpleDelimiter(char delim)
{
  m_delims.push_back(delim);
}

bool SimpleDelimiter::operator()(UniChar c) const
{
  return base::IsExist(m_delims, c);
}

UniChar LastUniChar(std::string const & s)
{
  if (s.empty())
    return 0;
  utf8::unchecked::iterator iter(s.end());
  --iter;
  return *iter;
}

bool to_size_t(char const * start, size_t & i, int base)
{
  uint64_t num = 0;
  if (!to_uint64(start, num, base))
    return false;

  i = static_cast<size_t>(num);
  return true;
}

bool to_float(char const * start, float & f)
{
  return ToReal(start, f);
}

bool to_double(char const * start, double & d)
{
  return ToReal(start, d);
}

UniString MakeLowerCase(UniString s)
{
  MakeLowerCaseInplace(s);
  return s;
}

void MakeLowerCaseInplace(std::string & s)
{
  UniString uniStr;
  utf8::unchecked::utf8to32(s.begin(), s.end(), std::back_inserter(uniStr));
  MakeLowerCaseInplace(uniStr);
  s.clear();
  utf8::unchecked::utf32to8(uniStr.begin(), uniStr.end(), back_inserter(s));
}

std::string MakeLowerCase(std::string s)
{
  MakeLowerCaseInplace(s);
  return s;
}

UniString Normalize(UniString s)
{
  NormalizeInplace(s);
  return s;
}

std::string Normalize(std::string const & s)
{
  auto uniString = MakeUniString(s);
  NormalizeInplace(uniString);
  return ToUtf8(uniString);
}

void NormalizeDigits(std::string & utf8)
{
  size_t const n = utf8.size();
  size_t const m = n >= 2 ? n - 2 : 0;

  size_t i = 0;
  while (i < n && utf8[i] != '\xEF')
    ++i;
  size_t j = i;

  // Following invariant holds before/between/after loop iterations below:
  // * utf8[0, i) represents a checked part of the input string.
  // * utf8[0, j) represents a normalized version of the utf8[0, i).
  while (i < m)
  {
    if (utf8[i] == '\xEF' && utf8[i + 1] == '\xBC')
    {
      auto const n = utf8[i + 2];
      if (n >= '\x90' && n <= '\x99')
      {
        utf8[j++] = n - 0x90 + '0';
        i += 3;
      }
      else
      {
        utf8[j++] = utf8[i++];
        utf8[j++] = utf8[i++];
      }
    }
    else
    {
      utf8[j++] = utf8[i++];
    }
  }
  while (i < n)
    utf8[j++] = utf8[i++];
  utf8.resize(j);
}

void NormalizeDigits(UniString & us)
{
  size_t const size = us.size();
  for (size_t i = 0; i < size; ++i)
  {
    UniChar const c = us[i];
    if (c >= 0xFF10 /* '０' */ && c <= 0xFF19 /* '９' */)
      us[i] = c - 0xFF10 + '0';
  }
}

void AsciiToLower(std::string & s)
{
  std::transform(s.begin(), s.end(), s.begin(), [](char in)
  {
    char constexpr diff = 'z' - 'Z';
    static_assert(diff == 'a' - 'A');
    static_assert(diff > 0);

    if (in >= 'A' && in <= 'Z')
      return char(in + diff);
    return in;
  });
}

void AsciiToUpper(std::string & s)
{
  std::transform(s.begin(), s.end(), s.begin(), [](char in)
  {
    char constexpr diff = 'z' - 'Z';
    static_assert(diff == 'a' - 'A');
    static_assert(diff > 0);

    if (in >= 'a' && in <= 'z')
      return char(in - diff);
    return in;
  });
}

void Trim(std::string & s)
{
  boost::trim_if(s, IsASCIISpace<std::string::value_type>);
}

void Trim(std::string_view & sv)
{
  auto const beg = std::find_if(sv.cbegin(), sv.cend(), [](auto c) { return !IsASCIISpace(c); });
  if (beg != sv.end())
  {
    auto const end = std::find_if(sv.crbegin(), sv.crend(), [](auto c) { return !IsASCIISpace(c); }).base();
    sv = std::string_view(sv.data() + std::distance(sv.begin(), beg), std::distance(beg, end));
  }
  else
    sv = {};
}

void Trim(std::string_view & s, std::string_view anyOf)
{
  auto i = s.find_first_not_of(anyOf);
  if (i != std::string_view::npos)
  {
    s.remove_prefix(i);

    i = s.find_last_not_of(anyOf);
    ASSERT(i != std::string_view::npos, ());
    s.remove_suffix(s.size() - i - 1);
  }
  else
    s = {};
}

void Trim(std::string & s, std::string_view anyOf)
{
  boost::trim_if(s, boost::is_any_of(anyOf));
}

bool ReplaceFirst(std::string & str, std::string const & from, std::string const & to)
{
  auto const pos = str.find(from);
  if (pos == std::string::npos)
    return false;

  str.replace(pos, from.length(), to);
  return true;
}

bool ReplaceLast(std::string & str, std::string const & from, std::string const & to)
{
  auto const pos = str.rfind(from);
  if (pos == std::string::npos)
    return false;

  str.replace(pos, from.length(), to);
  return true;
}

bool EqualNoCase(std::string const & s1, std::string const & s2)
{
  return MakeLowerCase(s1) == MakeLowerCase(s2);
}

UniString MakeUniString(std::string_view utf8s)
{
  UniString result;
  utf8::unchecked::utf8to32(utf8s.begin(), utf8s.end(), std::back_inserter(result));
  return result;
}

std::string ToUtf8(UniString const & s)
{
  std::string result;
  utf8::unchecked::utf32to8(s.begin(), s.end(), back_inserter(result));
  return result;
}

std::u16string ToUtf16(std::string_view utf8)
{
  std::u16string utf16;
  utf8::unchecked::utf8to16(utf8.begin(), utf8.end(), back_inserter(utf16));
  return utf16;
}

bool IsASCIIString(std::string_view sv)
{
  for (auto c : sv)
    if (c & 0x80)
      return false;
  return true;
}

bool IsASCIIDigit(UniChar c)
{
  return c >= '0' && c <= '9';
}

bool IsASCIILatin(UniChar c)
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool StartsWith(UniString const & s, UniString const & p)
{
  return StartsWith(s.begin(), s.end(), p.begin(), p.end());
}

bool EndsWith(UniString const & s1, UniString const & s2)
{
  if (s1.size() < s2.size())
    return false;

  return std::equal(s1.end() - s2.size(), s1.end(), s2.begin());
}

bool EatPrefix(std::string & s, std::string const & prefix)
{
  if (!s.starts_with(prefix))
    return false;

  CHECK_LESS_OR_EQUAL(prefix.size(), s.size(), ());
  s = s.substr(prefix.size());
  return true;
}

bool EatSuffix(std::string & s, std::string const & suffix)
{
  if (!s.ends_with(suffix))
    return false;

  CHECK_LESS_OR_EQUAL(suffix.size(), s.size(), ());
  s = s.substr(0, s.size() - suffix.size());
  return true;
}

std::string to_string_dac(double d, int dac)
{
  dac = std::min(std::numeric_limits<double>::digits10, dac);

  std::ostringstream ss;

  if (d < 1. && d > -1.)
  {
    std::string res;
    if (d >= 0.)
    {
      ss << std::setprecision(dac + 1) << d + 1;
      res = ss.str();
      res[0] = '0';
    }
    else
    {
      ss << std::setprecision(dac + 1) << d - 1;
      res = ss.str();
      res[1] = '0';
    }
    return res;
  }

  // Calc digits before comma (log10).
  double fD = fabs(d);
  while (fD >= 1.0 && dac < std::numeric_limits<double>::digits10)
  {
    fD /= 10.0;
    ++dac;
  }

  ss << std::setprecision(dac) << d;
  return ss.str();
}

bool IsHTML(std::string const & utf8)
{
  auto it = utf8.begin();
  size_t ltCount = 0;
  size_t gtCount = 0;
  while (it != utf8.end())
  {
    UniChar const c = utf8::unchecked::next(it);
    if (c == '<')
      ++ltCount;
    else if (c == '>')
      ++gtCount;
  }
  return (ltCount > 0 && gtCount > 0);
}

bool AlmostEqual(std::string const & str1, std::string const & str2, size_t mismatchedCount)
{
  std::pair mis(str1.begin(), str2.begin());
  auto const str1End = str1.end();
  auto const str2End = str2.end();

  for (size_t i = 0; i <= mismatchedCount; ++i)
  {
    auto const end = mis.first + std::min(distance(mis.first, str1End), distance(mis.second, str2End));
    mis = mismatch(mis.first, end, mis.second);
    if (mis.first == str1End && mis.second == str2End)
      return true;
    if (mis.first != str1End)
      ++mis.first;
    if (mis.second != str2End)
      ++mis.second;
  }
  return false;
}

void ParseCSVRow(std::string const & s, char const delimiter, std::vector<std::string> & target)
{
  target.clear();
  TokenizeIterator<SimpleDelimiter, std::string::const_iterator, true /* KeepEmptyTokens */> it(s.begin(), s.end(),
                                                                                                delimiter);
  for (; it; ++it)
  {
    std::string column(*it);
    Trim(column);
    target.push_back(std::move(column));
  }

  // Special case: if the string is empty, return an empty array instead of {""}.
  if (target.size() == 1 && target[0].empty())
    target.clear();
}

}  // namespace strings
