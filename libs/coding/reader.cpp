#include "coding/reader.hpp"

void Reader::ReadAsString(std::string & s) const
{
  s.clear();
  s.resize(static_cast<size_t>(Size()));
  Read(0 /* pos */, s.data(), s.size());
}
