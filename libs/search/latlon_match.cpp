#include "search/latlon_match.hpp"

#include "base/macros.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <string>
#include <utility>

namespace
{
std::string const kSpaces = " \t";
std::string const kCharsToSkip = " \n\t,;:.()";
std::string const kDecimalMarks = ".,";

bool IsDecimalMark(char c)
{
  return kDecimalMarks.find(c) != std::string::npos;
}

bool IsNegativeSymbol(char c)
{
  return c == '-';
}

template <typename Char>
void SkipSpaces(Char *& s)
{
  while (kSpaces.find(*s) != std::string::npos)
    ++s;
}

template <typename Char>
void Skip(Char *& s)
{
  while (kCharsToSkip.find(*s) != std::string::npos)
    ++s;
}

bool MatchDMSArray(char const *& s, char const * arr[], size_t count)
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

int GetDMSIndex(char const *& s)
{
  char const * arrDegree[] = {"*", "°"};
  char const * arrMinutes[] = {"\'", "’", "′"};
  char const * arrSeconds[] = {"\"", "”", "″", "\'\'", "’’", "′′"};

  if (MatchDMSArray(s, arrDegree, ARRAY_SIZE(arrDegree)))
    return 0;
  if (MatchDMSArray(s, arrSeconds, ARRAY_SIZE(arrSeconds)))
    return 2;
  if (MatchDMSArray(s, arrMinutes, ARRAY_SIZE(arrMinutes)))
    return 1;

  return -1;
}

bool SkipNSEW(char const *& s, char const * (&arrPos)[4])
{
  Skip(s);

  int ind;
  switch (*s)
  {
  case 'N':
  case 'n': ind = 0; break;
  case 'S':
  case 's': ind = 1; break;
  case 'E':
  case 'e': ind = 2; break;
  case 'W':
  case 'w': ind = 3; break;
  default: return false;
  }

  arrPos[ind] = s++;
  return true;
}

// Attempts to read a double from the start of |str|
// in one of what we assume are two most common forms
// for lat/lon: decimal digits separated either
// by a dot or by a comma, with digits on both sides
// of the separator.
// If the attempt fails, falls back to std::strtod.
double EatDouble(char const * str, char ** strEnd)
{
  bool gotDigitBeforeMark = false;
  bool gotMark = false;
  bool gotDigitAfterMark = false;
  char const * markPos = nullptr;
  char const * p = str;
  double modifier = 1.0;
  while (true)
  {
    if (IsDecimalMark(*p))
    {
      if (gotMark)
        break;
      gotMark = true;
      markPos = p;
    }
    else if (isdigit(*p))
    {
      if (gotMark)
        gotDigitAfterMark = true;
      else
        gotDigitBeforeMark = true;
    }
    else if (IsNegativeSymbol(*p))
    {
      modifier = -1.0;
    }
    else
    {
      break;
    }
    ++p;
  }

  if (gotDigitBeforeMark && gotMark && gotDigitAfterMark)
  {
    std::string const part1(str, markPos);
    std::string const part2(markPos + 1, p);
    *strEnd = const_cast<char *>(p);
    auto const x1 = atof(part1.c_str());
    auto const x2 = atof(part2.c_str());
    return x1 + x2 * modifier * pow(10.0, -static_cast<double>(part2.size()));
  }

  return strtod(str, strEnd);
}

bool SkipRequiredSpaces(char const *& s)
{
  char const * const start = s;
  SkipSpaces(s);
  return s != start;
}

bool IsCardinalDirection(char c)
{
  return c != 0 && strchr("NnSsEeWw", c) != nullptr;
}

bool MatchSpaceSeparatedDMS(std::string const & query, double & lat, double & lon)
{
  std::array<double, 6> values;
  std::array<char, 2> directions = {};
  char const * s = query.c_str();

  for (size_t coordinate = 0; coordinate < directions.size(); ++coordinate)
  {
    SkipSpaces(s);
    if (IsCardinalDirection(*s))
    {
      directions[coordinate] = *s++;
      if (!SkipRequiredSpaces(s))
        return false;
    }

    for (size_t part = 0; part < 3; ++part)
    {
      if (part > 0 && !SkipRequiredSpaces(s))
        return false;

      char * end;
      auto & value = values[coordinate * 3 + part];
      value = EatDouble(s, &end);
      if (s == end || value < 0.0 || !std::isfinite(value) || (part > 0 && value > 60.0))
        return false;
      s = end;
    }

    if (directions[coordinate] == 0)
    {
      SkipSpaces(s);
      if (!IsCardinalDirection(*s))
        return false;
      directions[coordinate] = *s++;
    }

    if (coordinate == 0 && !SkipRequiredSpaces(s))
      return false;
  }

  SkipSpaces(s);
  if (*s != 0)
    return false;

  std::array<bool, 2> assigned = {};
  for (size_t coordinate = 0; coordinate < directions.size(); ++coordinate)
  {
    char const direction = directions[coordinate];
    bool const isLatitude = strchr("NnSs", direction) != nullptr;
    size_t const axis = isLatitude ? 0 : 1;
    double value = values[coordinate * 3] + values[coordinate * 3 + 1] / 60.0 + values[coordinate * 3 + 2] / 3600.0;
    if (assigned[axis] || value > (isLatitude ? 90.0 : 180.0))
      return false;

    if (strchr("SsWw", direction) != nullptr)
      value = -value;
    (isLatitude ? lat : lon) = value;
    assigned[axis] = true;
  }
  return assigned[0] && assigned[1];
}
}  // namespace

namespace search
{
bool MatchLatLonDegree(std::string const & query, double & lat, double & lon)
{
  if (MatchSpaceSeparatedDMS(query, lat, lon))
    return true;

  // should be default initialization (0, false)
  std::array<std::pair<double, bool>, 6> v;

  int base = 0;

  // Positions of N, S, E, W symbols
  char const * arrPos[] = {nullptr, nullptr, nullptr, nullptr};
  bool arrDegreeSymbol[] = {false, false};

  char const * const startQuery = query.c_str();
  char const * s = startQuery;
  while (true)
  {
    char const * s1 = s;
    char const * s11 = s;
    if (SkipNSEW(s, arrPos))
    {
      s11 = s;
      Skip(s);
    }
    else
      SkipSpaces(s);

    if (!*s)
    {
      // End of the string - check matching.
      break;
    }

    char * s2;
    double const x = EatDouble(s, &s2);
    if (s == s2)
    {
      // invalid token
      if (s == s11)
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
    else if (x < 0 && s == s1 && !(s == startQuery || kSpaces.find(*(s - 1)) != std::string::npos))
    {
      // Skip input like "3-8"
      return false;
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

    if (i == 0)  // degrees
    {
      if (v[base].second)
      {
        if (base == 0)
        {
          base += 3;
        }
        else
        {
          // too many degree values
          return false;
        }
      }
      arrDegreeSymbol[base / 3] = degreeSymbol;
    }
    else                                    // minutes or seconds
      if (x < 0.0 || x > 60.0 ||            // minutes or seconds should be in [0, 60] range
          v[base + i].second ||             // value already exists
          !v[base].second ||                // no degrees found for value
          (i == 2 && !v[base + 1].second))  // no minutes for seconds
      {
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
  if (v[0].first < 0.0)
    lat = -lat;

  lon = fabs(v[3].first) + v[4].first / 60.0 + v[5].first / 3600.0;
  if (v[3].first < 0.0)
    lon = -lon;

  if (std::max(arrPos[0], arrPos[1]) > std::max(arrPos[2], arrPos[3]))
    std::swap(lat, lon);

  if (arrPos[1] != nullptr)
    lat = -lat;
  if (arrPos[3] != nullptr)
    lon = -lon;

  // Valid input ranges for longitude are: [0, 360] or [-180, 180].
  // We normalize it to [-180, 180].
  if (lon < -180.0 || lon > 360.0)
    return false;

  if (lon > 180.0)
    lon -= 360.0;

  return fabs(lat) <= 90.0;
}
}  // namespace search
