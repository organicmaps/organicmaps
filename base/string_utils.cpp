#include "string_utils.hpp"
#include "assert.hpp"

#include "../std/target_os.hpp"
#include "../std/iterator.hpp"

#include <boost/algorithm/string.hpp> // boost::trim


namespace strings
{

SimpleDelimiter::SimpleDelimiter(char const * delimChars)
{
  string const s(delimChars);
  string::const_iterator it = s.begin();
  while (it != s.end())
    m_delims.push_back(utf8::unchecked::next(it));
}

bool SimpleDelimiter::operator()(UniChar c) const
{
  for (UniString::const_iterator it = m_delims.begin(); it != m_delims.end(); ++it)
    if (*it == c)
      return true;
  return false;
}

UniChar LastUniChar(string const & s)
{
  if (s.empty())
    return 0;
  utf8::unchecked::iterator<string::const_iterator> iter(s.end());
  --iter;
  return *iter;
}

bool to_int(char const * s, int & i)
{
  char * stop;
  long const x = strtol(s, &stop, 10);
  if (stop && *stop == 0)
  {
    i = static_cast<int>(x);
    ASSERT_EQUAL(static_cast<long>(i), x, ());
    return true;
  }
  return false;
}

bool to_uint64(char const * s, uint64_t & i)
{
  char * stop;
#ifdef OMIM_OS_WINDOWS_NATIVE
  i = _strtoui64(s, &stop, 10);
#else
  i = strtoull(s, &stop, 10);
#endif
  return stop && *stop == 0;
}

bool to_int64(char const * s, int64_t & i)
{
  char * stop;
#ifdef OMIM_OS_WINDOWS_NATIVE
  i = _strtoi64(s, &stop, 10);
#else
  i = strtoll(s, &stop, 10);
#endif
  return stop && *stop == 0;
}

bool to_double(char const * s, double & d)
{
  char * stop;
  d = strtod(s, &stop);
  return stop && *stop == 0;
}

UniString MakeLowerCase(UniString const & s)
{
  UniString result(s);
  MakeLowerCase(result);
  return result;
}

void MakeLowerCase(string & s)
{
  UniString uniStr;
  utf8::unchecked::utf8to32(s.begin(), s.end(), back_inserter(uniStr));
  MakeLowerCase(uniStr);
  s.clear();
  utf8::unchecked::utf32to8(uniStr.begin(), uniStr.end(), back_inserter(s));
}

string MakeLowerCase(string const & s)
{
  string result(s);
  MakeLowerCase(result);
  return result;
}

UniString Normalize(UniString const & s)
{
  UniString result(s);
  Normalize(result);
  return result;
}

namespace
{
  char ascii_to_lower(char in)
  {
    static char const diff = 'Z'-'z';
    if (in <= 'Z' && in >= 'A')
      return (in-diff);
    return in;
  }
}

void AsciiToLower(string & s)
{
  transform(s.begin(), s.end(), s.begin(), &ascii_to_lower);
}

void Trim(string & s)
{
  boost::trim(s);
}

bool EqualNoCase(string const & s1, string const & s2)
{
  return MakeLowerCase(s1) == MakeLowerCase(s2);
}

UniString MakeUniString(string const & utf8s)
{
  UniString result;
  utf8::unchecked::utf8to32(utf8s.begin(), utf8s.end(), back_inserter(result));
  return result;
}

string ToUtf8(UniString const & s)
{
  string result;
  utf8::unchecked::utf32to8(s.begin(), s.end(), back_inserter(result));
  return result;
}

bool IsASCIIString(string const & str)
{
  for (size_t i = 0; i < str.size(); ++i)
    if (str[i] & 0x80)
      return false;
  return true;
}

bool StartsWith(string const & s1, char const * s2)
{
  return (s1.compare(0, strlen(s2), s2) == 0);
}

}
