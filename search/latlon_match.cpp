#include "latlon_match.hpp"
#include "../std/algorithm.hpp"
#include "../std/cmath.hpp"
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

bool search::MatchLatLon(string const & str, double & latRes, double & lonRes,
                         double & precisionLat, double & precisionLon)
{
  char const * s = str.c_str();

  Skip(s);
  if (!*s)
    return false;

  char * s1;
  double const lat = strtod(s, &s1);
  int latDigits = s1 - s - 2;
  if (lat < 0)
    --latDigits;
  if (fabs(lat) >= 10)
    --latDigits;
  if (lat < -90 || lat > 90)
    return false;

  if (!s1 || !*s1)
    return false;
  Skip(s1);
  if (!*s1)
    return false;

  char * s2;
  double lon = strtod(s1, &s2);
  int lonDigits = s2 - s1 - 2;
  if (lon < 0)
    --lonDigits;
  if (fabs(lon) >= 10)
    --lonDigits;
  if (fabs(lon) >= 100)
    --lonDigits;
  if (lon < -180 || lon > 360)
    return false;
  if (lon > 180)
    lon = lon - 360;

  if (!s2)
    return false;
  Skip(s2);
  if (*s2)
    return false;

  latRes = lat;
  lonRes = lon;
  precisionLat = 0.5 * pow(10.0, -max(0, min(8, latDigits)));
  precisionLon = 0.5 * pow(10.0, -max(0, min(8, lonDigits)));
  return true;
}
