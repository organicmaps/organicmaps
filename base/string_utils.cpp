#include "base/string_utils.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iterator>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-local-typedef"
#endif

#include <boost/algorithm/string/trim.hpp>

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

namespace strings
{
namespace
{
template <typename T>
T RealConverter(char const * start, char ** stop);

template <>
float RealConverter<float>(char const * start, char ** stop)
{
  return std::strtof(start, stop);
}

template <>
double RealConverter<double>(char const * start, char ** stop)
{
  return std::strtod(start, stop);
}

template <typename T>
bool ToReal(char const * start, T & result)
{
  char * stop;
  auto const tmp = RealConverter<T>(start, &stop);

  if (*stop != 0 || start == stop || !std::isfinite(tmp))
    return false;

  result = tmp;
  return true;
}

}  // namespace

bool UniString::IsEqualAscii(char const * s) const
{
  return (size() == strlen(s) && std::equal(begin(), end(), s));
}

SimpleDelimiter::SimpleDelimiter(char const * delims)
{
  std::string const s(delims);
  std::string::const_iterator it = s.begin();
  while (it != s.end())
    m_delims.push_back(utf8::unchecked::next(it));
}

SimpleDelimiter::SimpleDelimiter(char delim) { m_delims.push_back(delim); }

bool SimpleDelimiter::operator()(UniChar c) const
{
  return std::find(m_delims.begin(), m_delims.end(), c) != m_delims.end();
}

UniChar LastUniChar(std::string const & s)
{
  if (s.empty())
    return 0;
  utf8::unchecked::iterator<std::string::const_iterator> iter(s.end());
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

UniString MakeLowerCase(UniString const & s)
{
  UniString result(s);
  MakeLowerCaseInplace(result);
  return result;
}

void MakeLowerCaseInplace(std::string & s)
{
  UniString uniStr;
  utf8::unchecked::utf8to32(s.begin(), s.end(), std::back_inserter(uniStr));
  MakeLowerCaseInplace(uniStr);
  s.clear();
  utf8::unchecked::utf32to8(uniStr.begin(), uniStr.end(), back_inserter(s));
}

std::string MakeLowerCase(std::string const & s)
{
  std::string result(s);
  MakeLowerCaseInplace(result);
  return result;
}

UniString Normalize(UniString const & s)
{
  UniString result(s);
  NormalizeInplace(result);
  return result;
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

namespace
{
char ascii_to_lower(char in)
{
  char const diff = 'z' - 'Z';
  static_assert(diff == 'a' - 'A', "");
  static_assert(diff > 0, "");

  if (in >= 'A' && in <= 'Z')
    return (in + diff);
  return in;
}
}  // namespace

void AsciiToLower(std::string & s) { transform(s.begin(), s.end(), s.begin(), &ascii_to_lower); }

std::string & TrimLeft(std::string & s)
{
  s.erase(s.begin(), std::find_if(s.cbegin(), s.cend(), [](auto c) { return !std::isspace(c); }));
  return s;
}

std::string & TrimRight(std::string & s)
{
  s.erase(std::find_if(s.crbegin(), s.crend(), [](auto c) { return !std::isspace(c); }).base(),
          s.end());
  return s;
}

std::string & Trim(std::string & s) { return TrimLeft(TrimRight(s)); }

std::string & Trim(std::string & s, char const * anyOf)
{
  boost::trim_if(s, boost::is_any_of(anyOf));
  return s;
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

UniString MakeUniString(std::string const & utf8s)
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

bool IsASCIIString(std::string const & str)
{
  for (size_t i = 0; i < str.size(); ++i)
    if (str[i] & 0x80)
      return false;
  return true;
}

bool IsASCIIDigit(UniChar c) { return c >= '0' && c <= '9'; }

bool IsASCIISpace(UniChar c)
{
  return c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v';
}

bool IsASCIINumeric(std::string const & str)
{
  if (str.empty())
    return false;
  return std::all_of(str.begin(), str.end(), strings::IsASCIIDigit);
}

bool IsASCIILatin(UniChar c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }

bool StartsWith(UniString const & s, UniString const & p)
{
  return StartsWith(s.begin(), s.end(), p.begin(), p.end());
}

bool StartsWith(std::string const & s1, char const * s2)
{
  return (s1.compare(0, strlen(s2), s2) == 0);
}

bool StartsWith(std::string const & s1, std::string const & s2)
{
  return (s1.compare(0, s2.length(), s2) == 0);
}

bool EndsWith(UniString const & s1, UniString const & s2)
{
  if (s1.size() < s2.size())
    return false;

  return std::equal(s1.end() - s2.size(), s1.end(), s2.begin());
}

bool EndsWith(std::string const & s1, char const * s2)
{
  size_t const n = s1.size();
  size_t const m = strlen(s2);
  if (n < m)
    return false;
  return (s1.compare(n - m, m, s2) == 0);
}

bool EndsWith(std::string const & s1, std::string const & s2)
{
  return s1.size() >= s2.size() && s1.compare(s1.size() - s2.size(), s2.size(), s2) == 0;
}

bool EatPrefix(std::string & s, std::string const & prefix)
{
  if (!StartsWith(s, prefix))
    return false;

  CHECK_LESS_OR_EQUAL(prefix.size(), s.size(), ());
  s = s.substr(prefix.size());
  return true;
}

bool EatSuffix(std::string & s, std::string const & suffix)
{
  if (!EndsWith(s, suffix))
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
  std::string::const_iterator it = utf8.begin();
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
  std::pair<std::string::const_iterator, std::string::const_iterator> mis(str1.begin(),
                                                                          str2.begin());
  auto const str1End = str1.end();
  auto const str2End = str2.end();

  for (size_t i = 0; i <= mismatchedCount; ++i)
  {
    auto const end =
        mis.first + std::min(distance(mis.first, str1End), distance(mis.second, str2End));
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
  using It = TokenizeIterator<SimpleDelimiter, std::string::const_iterator, true>;
  for (It it(s, SimpleDelimiter(delimiter)); it; ++it)
  {
    std::string column = *it;
    strings::Trim(column);
    target.push_back(move(column));
  }

  // Special case: if the string is empty, return an empty array instead of {""}.
  if (target.size() == 1 && target[0].empty())
    target.clear();
}

}  // namespace strings
