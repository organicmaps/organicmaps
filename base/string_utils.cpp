#include "string_utils.hpp"
#include "assert.hpp"

#include "../std/sstream.hpp"

#include <locale>   // for make_lower_case

namespace strings
{

TokenizeIterator::TokenizeIterator(string const & s, char const * delim)
: m_start(0), m_src(s), m_delim(delim)
{
  move();
}

void TokenizeIterator::move()
{
  m_end = m_src.find_first_of(m_delim, m_start);
  if (m_end == string::npos) m_end = m_src.size();
}

string TokenizeIterator::operator*() const
{
  ASSERT ( !end(), ("dereference of empty iterator") );
  return m_src.substr(m_start, m_end - m_start);
}

TokenizeIterator & TokenizeIterator::operator++()
{
  m_start = m_end + 1;
  move();
  return (*this);
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
