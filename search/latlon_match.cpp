#include "search/latlon_match.hpp"

#include "base/macros.hpp"

#include "std/array.hpp"
#include "std/cmath.hpp"
#include "std/cstdlib.hpp"
#include "std/cstring.hpp"
#include "std/algorithm.hpp"
#include "std/utility.hpp"


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

namespace
{

bool MatchDMSArray(char const * & s, char const * arr[], size_t count)
{
  for (size_t i = 0; i < count; ++i)
  {
    size_t const len = strlen(arr[i]);
    if (strncmp(s, arr[i], len) == 0)
    {
      s += len;
      return true;
    }
  }
  return false;
}

int GetDMSIndex(char const * & s)
{
  char const * arrDegree[] = { "*", "°" };
  char const * arrMinutes[] = { "\'", "’", "′" };
  char const * arrSeconds[] = { "\"", "”", "″", "\'\'", "’’", "′′" };

  if (MatchDMSArray(s, arrDegree, ARRAY_SIZE(arrDegree)))
      return 0;
  if (MatchDMSArray(s, arrSeconds, ARRAY_SIZE(arrSeconds)))
    return 2;
  if (MatchDMSArray(s, arrMinutes, ARRAY_SIZE(arrMinutes)))
    return 1;

  return -1;
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
  bool arrDegreeSymbol[] = { false, false };

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
      // invalid token
      if (s == s1)
      {
        // Return error if there are no any delimiters.
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

    int i = GetDMSIndex(s);
    bool degreeSymbol = true;
    if (i == -1)
    {
      // try to assign next possible value mark
      if (arrDegreeSymbol[base / 3])
      {
        if (!v[base + 1].second)
          i = 1;
        else
          i = 2;
      }
      else
      {
        i = 0;
        degreeSymbol = false;
      }
    }

    if (i == 0) // degrees
    {
      if (v[base].second)
      {
        if (base == 0)
          base += 3;
        else
        {
          // too many degree values
          return false;
        }
      }
      arrDegreeSymbol[base / 3] = degreeSymbol;
    }
    else  // minutes or seconds
    {
      if (x < 0.0 || x > 60.0 ||            // minutes or seconds should be in [0, 60] range
          v[base + i].second ||             // value already exists
          !v[base].second ||                // no degrees found for value
          (i == 2 && !v[base + 1].second))  // no minutes for seconds
      {
        return false;
      }
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

  // Valid input ranges for longitude are: [0, 360] or [-180, 180].
  // We do normilize it to [-180, 180].
  if (lon > 180.0)
  {
    if (lon > 360.0)
      return false;
    lon -= 360.0;
  }
  else if (lon < -180.0)
    return false;

  return (fabs(lat) <= 90.0);
}

} // search
