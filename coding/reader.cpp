#include "coding/reader.hpp"

#include "base/string_utils.hpp"

void Reader::ReadAsString(std::string & s) const
{
  s.clear();
  size_t const sz = static_cast<size_t>(Size());
  s.resize(sz);
  Read(0, &s[0], sz);
}

std::vector<uint8_t> Reader::ReadAsBytes() const
{
  std::vector<uint8_t> contents;
  contents.resize(static_cast<size_t>(Size()));
  Read(0 /* pos */, contents.data(), contents.size());
  return contents;
}

bool Reader::IsEqual(std::string const & name1, std::string const & name2)
{
#if defined(OMIM_OS_WINDOWS)
  return strings::EqualNoCase(name1, name2);
#else
  return (name1 == name2);
#endif
}
