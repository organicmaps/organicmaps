#include "latlon_match.hpp"
#include "../std/cstdlib.hpp"

namespace
{

template <typename CharT> void Skip(CharT * & s)
{
  // 0xC2 - commond Unicode first byte for ANSI (not ASCII!) symbols.
  // 0xC2,0xB0 - degree symbol.
  while (*s == ' ' || *s == ',' || *s == ';' || *s == ':' || *s == '.' || *s == '(' || *s == ')' ||
         *s == char(0xB0) || *s == char(0xC2))
    ++s;
}

}  // unnamed namespace

bool search::MatchLatLon(string const & str, double & latRes, double & lonRes)
{
  char const * s = str.c_str();

  Skip(s);
  if (!*s)
    return false;

  char * s1;
  double const lat = strtod(s, &s1);
  if (!s1 || !*s1)
    return false;

  Skip(s1);
  if (!*s1)
    return false;

  char * s2;
  double lon = strtod(s1, &s2);
  if (!s2)
    return false;

  Skip(s2);
  if (*s2)
    return false;

  if (lat < -90 || lat > 90)
    return false;
  if (lon < -180 || lon > 360)
    return false;
  if (lon > 180)
    lon = lon - 360;

  latRes = lat;
  lonRes = lon;
  return true;
}
