#include "search/latlon_match.hpp"

#include "base/macros.hpp"
#include "base/math.hpp"
#include "base/string_utils.hpp"

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

bool HasDMSSymbolAt(char const * s)
{
  char const * p = s;
  return GetDMSIndex(p) >= 0;
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
    else if (strings::IsASCIIDigit(*p))
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

bool IsHemisphereChar(char c)
{
  switch (c)
  {
  case 'N': case 'n': case 'S': case 's':
  case 'E': case 'e': case 'W': case 'w':
    return true;
  default:
    return false;
  }
}

// Match one N/S/E/W letter. Advances |s|. Returns 0N/1S/2E/3W or -1.
int MatchHemisphere(char const *& s)
{
  SkipSpaces(s);
  int ind = -1;
  switch (*s)
  {
  case 'N': case 'n': ind = 0; break;
  case 'S': case 's': ind = 1; break;
  case 'E': case 'e': ind = 2; break;
  case 'W': case 'w': ind = 3; break;
  default: return -1;
  }
  ++s;
  return ind;
}

// Symbol-free hemisphere-qualified group with a fixed part count (2 or 3).
// Grammar: [H] D M [S] [H]  — exactly one hemisphere (prefix or suffix).
// DMS symbols are not consumed; those forms stay on the legacy parser.
bool TryParseGroupFixedParts(char const *& s, int nParts, double & signedDegrees, int & hemOut)
{
  if (nParts < 2 || nParts > 3)
    return false;

  char const * const start = s;
  Skip(s);

  int hemPrefix = -1;
  {
    char const * const before = s;
    SkipSpaces(s);
    int ind = -1;
    switch (*s)
    {
    case 'N': case 'n': ind = 0; break;
    case 'S': case 's': ind = 1; break;
    case 'E': case 'e': ind = 2; break;
    case 'W': case 'w': ind = 3; break;
    default: break;
    }
    if (ind >= 0)
    {
      char const * probe = s + 1;
      SkipSpaces(probe);
      // Reject if a DMS symbol sits where a number should be.
      if (HasDMSSymbolAt(probe))
      {
        s = start;
        return false;
      }
      char * end = nullptr;
      EatDouble(probe, &end);
      if (end == probe)
        s = before;
      else
      {
        ++s;  // consume hemisphere
        hemPrefix = ind;
      }
    }
  }

  double parts[3] = {};
  for (int i = 0; i < nParts; ++i)
  {
    SkipSpaces(s);
    if (!*s)
    {
      s = start;
      return false;
    }
    // New grammar stays symbol-free.
    if (HasDMSSymbolAt(s))
    {
      s = start;
      return false;
    }
    if (IsHemisphereChar(*s))
    {
      s = start;
      return false;
    }
    if (IsDecimalMark(*s))
    {
      s = start;
      return false;
    }

    char const * const numStart = s;
    char * end = nullptr;
    double const x = EatDouble(s, &end);
    if (end == numStart)
    {
      s = start;
      return false;
    }
    // Reject multi-mark components (e.g. 1.2.3) but allow a coordinate
    // delimiter after a complete number (e.g. "27.73, W ...").
    if (IsDecimalMark(*end) && strings::IsASCIIDigit(*(end + 1)))
    {
      s = start;
      return false;
    }
    if (!math::is_finite(x))
    {
      s = start;
      return false;
    }
    parts[i] = x;
    s = end;
  }

  // Optional trailing hemisphere when there was no prefix.
  int hemSuffix = -1;
  if (hemPrefix < 0)
  {
    char const * const before = s;
    SkipSpaces(s);
    int ind = -1;
    switch (*s)
    {
    case 'N': case 'n': ind = 0; break;
    case 'S': case 's': ind = 1; break;
    case 'E': case 'e': ind = 2; break;
    case 'W': case 'w': ind = 3; break;
    default: break;
    }
    if (ind < 0)
    {
      s = start;
      return false;
    }
    ++s;
    hemSuffix = ind;
  }

  int const hem = (hemPrefix >= 0) ? hemPrefix : hemSuffix;
  if (hem < 0)
  {
    s = start;
    return false;
  }

  // Degrees may be signed only for the first component when no hemisphere
  // would otherwise supply the sign; with a required hemisphere we reject
  // negative degrees/minutes/seconds forms like "N -51 30".
  if (parts[0] < 0.0 || parts[1] < 0.0 || parts[1] >= 60.0)
  {
    s = start;
    return false;
  }
  if (nParts >= 3 && (parts[2] < 0.0 || parts[2] >= 60.0))
  {
    s = start;
    return false;
  }

  double absDeg = parts[0] + parts[1] / 60.0;
  if (nParts >= 3)
    absDeg += parts[2] / 3600.0;

  if (!math::is_finite(absDeg))
  {
    s = start;
    return false;
  }

  bool const isLat = (hem == 0 || hem == 1);
  if (isLat && absDeg > 90.0)
  {
    s = start;
    return false;
  }
  if (!isLat && absDeg > 180.0)
  {
    s = start;
    return false;
  }

  signedDegrees = absDeg;
  if (hem == 1 || hem == 3)
    signedDegrees = -signedDegrees;
  hemOut = hem;
  return true;
}

// Two symbol-free hemisphere-qualified D M [S] groups, full query consumed.
// If several part-count splits succeed with different coordinates, reject as
// ambiguous (e.g. "N 51 30 0 7 W").
bool TryMatchTwoHemisphereGroups(char const * query, double & lat, double & lon)
{
  struct Result
  {
    double lat;
    double lon;
  };
  Result found {};
  int foundCount = 0;

  int const partChoices[2] = {2, 3};
  for (int n0 : partChoices)
  {
    for (int n1 : partChoices)
    {
      char const * s = query;
      double v0 = 0.0;
      double v1 = 0.0;
      int h0 = -1;
      int h1 = -1;

      if (!TryParseGroupFixedParts(s, n0, v0, h0))
        continue;
      if (!TryParseGroupFixedParts(s, n1, v1, h1))
        continue;

      bool const lat0 = (h0 == 0 || h0 == 1);
      bool const lat1 = (h1 == 0 || h1 == 1);
      if (lat0 == lat1)
        continue;

      // Full query must be consumed (delimiters only).
      Skip(s);
      if (*s != '\0')
        continue;

      double la = lat0 ? v0 : v1;
      double lo = lat0 ? v1 : v0;

      if (std::fabs(la) > 90.0)
        continue;
      if (lo < -180.0 || lo > 360.0)
        continue;
      if (lo > 180.0)
        lo -= 360.0;
      if (!math::is_finite(la) || !math::is_finite(lo))
        continue;

      if (foundCount == 0)
      {
        found = {la, lo};
        foundCount = 1;
      }
      else if (std::fabs(found.lat - la) > 1e-12 || std::fabs(found.lon - lo) > 1e-12)
      {
        // Ambiguous split between groups.
        return false;
      }
    }
  }

  if (foundCount != 1)
    return false;

  lat = found.lat;
  lon = found.lon;
  return true;
}

namespace search
{
bool MatchLatLonDegree(std::string const & query, double & lat, double & lon)
{
  // Prefer two complete hemisphere-qualified groups (space-separated DMS or
  // decimal-with-suffix). Fall back to the historical free-form parser.
  if (TryMatchTwoHemisphereGroups(query.c_str(), lat, lon))
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
