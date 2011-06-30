#include "reader.hpp"

#include "../../base/string_utils.hpp"


void Reader::ReadAsString(string & s) const
{
  s.clear();
  size_t const sz = Size();
  s.resize(sz);
  Read(0, &s[0], sz);
}

bool ModelReader::IsEqual(string const & fName) const
{
#if defined(OMIM_OS_WINDOWS)
  return strings::EqualNoCase(fName, m_name);
#else
  return (fName == m_name);
#endif
}
