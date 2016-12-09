#include "coding/reader.hpp"

#include "base/string_utils.hpp"


void Reader::ReadAsString(string & s) const
{
  s.clear();
  size_t const sz = static_cast<size_t>(Size());
  s.resize(sz);
  Read(0, &s[0], sz);
}

bool Reader::IsEqual(string const & name1, string const & name2)
{
#if defined(OMIM_OS_WINDOWS)
  return strings::EqualNoCase(name1, name2);
#else
  return (name1 == name2);
#endif
}
