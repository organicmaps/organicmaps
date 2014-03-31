#include "latlon_match.hpp"

#include "../indexer/mercator.hpp"

#include "../std/array.hpp"
#include "../std/cmath.hpp"
#include "../std/cstdlib.hpp"


namespace search
{

namespace
{

template <typename CharT> void SkipSpaces(CharT * & s)
{
  while (*s && (*s == ' ' || *s == '\t'))
    ++s;
}

template <typename CharT> void Skip(CharT * & s)
{
  while (*s && (*s == ' ' || *s == '\t' || *s == ',' || *s == ';' ||
                *s == ':' || *s == '.' || *s == '(' || *s == ')'))
    ++s;
}

}  // unnamed namespace

bool MatchLatLon(string const & str, double & latRes, double & lonRes,
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

namespace
{

int Match1Byte(char const * & s)
{
  uint8_t const ch = static_cast<uint8_t>(*s++);
  switch (ch)
  {
  case 0xB0:    // °
    return 0;
  default:
    return -1;
  }
}

int Match2Bytes(char const * & s)
{
  uint8_t ch = static_cast<uint8_t>(*s++);
  if (ch != 0x80)
    return -1;

  ch = static_cast<uint8_t>(*s++);
  switch (ch)
  {
  case 0x99:  // ’
    return 1;
  case 0x9D:  // ”
    return 2;
  case 0xB2:  // ′
    if (static_cast<uint8_t>(*s) == 0xE2
        && static_cast<uint8_t>(*(s+1)) == 0x80
        && static_cast<uint8_t>(*(s+2)) == 0xB2)
    {
      s += 3;
      return 2;  // this specific case when the string is normalized and ″ is splitted to ′′
    }
    return 1;
  case 0xB3:  // ″
    return 2;
  default:
    return -1;
  }
}

int GetDMSIndex(char const * & s)
{
  uint8_t const ch = static_cast<uint8_t>(*s++);
  switch (ch)
  {
  // UTF8 control symbols
  case 0xC2:
    return Match1Byte(s);
  case 0xE2:
    return Match2Bytes(s);

  case '*': return 0;
  case '\'': return 1;
  case '\"': return 2;
  default: return -1;
  }
}

void SkipNSEW(char const * & s, char const * (&arrPos) [4])
{
  Skip(s);

  int ind;
  switch (*s)
  {
  case 'N': case 'n': ind = 0; break;
  case 'S': case 's': ind = 1; break;
  case 'E': case 'e': ind = 2; break;
  case 'W': case 'w': ind = 3; break;
  default: return;
  }

  arrPos[ind] = s++;

  Skip(s);
}

}

bool MatchLatLonDegree(string const & query, double & lat, double & lon)
{
  // should be default initialization (0, false)
  array<pair<double, bool>, 6> v;

  int base = 0;

  // Positions of N, S, E, W symbols
  char const * arrPos[] = { 0, 0, 0, 0 };

  char const * s = query.c_str();
  while (true)
  {
    char const * s1 = s;
    SkipNSEW(s, arrPos);
    if (!*s)
    {
      // End of the string - check matching.
      break;
    }

    char * s2;
    double const x = strtod(s, &s2);
    if (s == s2)
    {
      // Invalid token
      if (s == s1)
      {
        // Return error if there are no any delimiters
        return false;
      }
      else
      {
        // Check matching if token is delimited.
        break;
      }
    }

    s = s2;
    SkipSpaces(s);

    int const i = GetDMSIndex(s);
    switch (i)
    {
    case -1:  // expect valid control symbol
      return false;

    case 0:   // degree
      if (v[base].second)
      {
        if (base == 0)
          base += 3;
        else
        {
          // repeated value
          return false;
        }
      }
      break;

    default:  // minutes or seconds
      if (x < 0.0 || v[base + i].second || !v[base].second)
        return false;
    }

    v[base + i].first = x;
    v[base + i].second = true;
  }

  if (!v[0].second || !v[3].second)
  {
    // degree should exist for both coordinates
    return false;
  }

  if ((arrPos[0] && arrPos[1]) || (arrPos[2] && arrPos[3]))
  {
    // control symbols should match only once
    return false;
  }

  // Calculate Lat, Lon with correct sign.
  lat = fabs(v[0].first) + v[1].first / 60.0 + v[2].first / 3600.0;
  if (v[0].first < 0.0) lat = -lat;

  lon = fabs(v[3].first) + v[4].first / 60.0 + v[5].first / 3600.0;
  if (v[3].first < 0.0) lon = -lon;

  if (max(arrPos[0], arrPos[1]) > max(arrPos[2], arrPos[3]))
    swap(lat, lon);

  if (arrPos[1]) lat = -lat;
  if (arrPos[3]) lon = -lon;

  if (lon > 180.0) lon -= 360.0;
  if (lon < -180.0) lon += 360.0;

  return MercatorBounds::ValidLat(lat) && MercatorBounds::ValidLon(lon);
}

} // search
