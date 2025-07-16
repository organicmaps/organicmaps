#pragma once

#include "base/assert.hpp"
#include "base/base.hpp"

#include <cstdint>
#include <string>

namespace stream
{
namespace detail
{
template <class TStream>
void ReadString(TStream & s, std::string & t)
{
  uint32_t count;
  s >> count;
  t.reserve(count);

  while (count > 0)
  {
    char c;
    s >> c;
    t.push_back(c);
    --count;
  }
}

template <class TStream, class TWriter>
void WriteString(TStream & s, TWriter & w, std::string const & t)
{
  uint32_t const count = static_cast<uint32_t>(t.size());
  s << count;
  if (count > 0)
    w.Write(t.c_str(), count);
}

template <class TStream>
void ReadBool(TStream & s, bool & t)
{
  char tt;
  s >> tt;
  ASSERT(tt == 0 || tt == 1, (tt));
  t = (tt != 0);
}

template <class TStream>
void WriteBool(TStream & s, bool t)
{
  s << char(t ? 1 : 0);
}
}  // namespace detail
}  // namespace stream
