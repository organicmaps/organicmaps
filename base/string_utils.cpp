#include "string_utils.hpp"
#include "assert.hpp"

#include "../std/sstream.hpp"
#include "../std/iterator.hpp"

#include <locale>   // for make_lower_case

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
  int x = strtol(s, &stop, 10);
  if (stop && *stop == 0)
  {
    i = x;
    return true;
  }
  return false;
}

bool to_uint64(char const * s, uint64_t & i)
{
  istringstream ss;
  ss.str(s);
  ss >> i;
  return !ss.fail();
}

bool to_int64(char const * s, int64_t & i)
{
  istringstream ss;
  ss.str(s);
  ss >> i;
  return !ss.fail();
}

bool to_double(char const * s, double & d)
{
  char * stop;
  double x = strtod(s, &stop);
  if (stop && *stop == 0)
  {
    d = x;
    return true;
  }
  return false;
}

void make_lower_case(string & s)
{
  if (!s.empty())
  {
    std::locale l;
    std::use_facet<std::ctype<char> >(l).tolower(&s[0], &s[0] + s.size());
  }
}

bool equal_no_case(string s1, string s2)
{
  make_lower_case(s1);
  make_lower_case(s2);
  return (s1 == s2);
}

}
