#include "coding/reader.hpp"

void Reader::ReadAsString(std::string & s) const
{
  s.clear();
  size_t const sz = static_cast<size_t>(Size());
  s.resize(sz);
  Read(0, &s[0], sz);
}
