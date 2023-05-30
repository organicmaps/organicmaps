#include "search/utm_mgrs_coords_match.hpp"

#include "base/string_utils.hpp"

#include "platform/utm_mgrs_utils.hpp"

#include <iterator>
#include <string>
#include <utility>
#include <math.h>

#include "base/math.hpp"
#include "base/string_utils.hpp"

namespace search
{

using namespace std;

string const kSpaceChars = " \t\r";


// Matches 2 digits zone code. Returns end position of matched chars or string::npos if no match.
size_t MatchZoneCode(string const & query, int & zone_code)
{
  auto const pos = query.find_first_not_of(kSpaceChars);
  if (query.size() - pos < 2)
    return string::npos;

  char const dig1 = query[pos];
  char const dig2 = query[pos+1];
  if (dig1 < '0' || dig1 > '9' || dig2 < '0' || dig2 > '9')
    return string::npos;

  zone_code = (dig1 - '0') * 10 + (dig2 - '0');
  return pos + 2;
}


// Matches zone letter ignoring spaces. Returns end position of matched chars or string::npos if no match.
size_t MatchZoneLetter(string const & query, char & zone_letter, size_t startPos)
{
  auto const pos = query.find_first_not_of(kSpaceChars, startPos);
  if (query.size() == pos)
    return string::npos;

  char const l = query[pos];
  if (l < 'A' || l > 'Z')
    return string::npos;

  zone_letter = l;
  return pos + 1;
}

// Matches long number ignoring spaces. Returns end position of matched chars or string::npos if no match.
size_t MatchInt(string const & query, int & value, size_t startPos)
{
  auto pos = query.find_first_not_of(kSpaceChars, startPos);
  if (query.size() == pos)
    return string::npos;

  int n = 0;
  while (pos < query.size())
  {
    char ch = query[pos];
    if (ch >= '0' && ch <= '9') // Found digit
    {
      n = n * 10 + (ch - '0');
      pos ++;
    }
    else if (kSpaceChars.find(ch) != string::npos) // Found space char matching end of the number
      break;
    else // Found invalid char
      return string::npos;
  }

  value = n;
  return pos;
}

// Parse UTM format "(\d\d)\s?(\W)\s+(\d+)\s+(\d+)" and converts it to lat,lon.
// Return true if parsed successfully or false otherwise.
// See utm_mgrs_coords_match_test.cpp for sample UTM strings
std::optional<ms::LatLon> MatchUTMCoords(string const & query)
{
  int  easting, northing;
  int  zone_code;
  char zone_letter;

  size_t pos = MatchZoneCode(query, zone_code);
  if (pos == string::npos)
    return nullopt;

  pos = MatchZoneLetter(query, zone_letter, pos);
  if (pos == string::npos)
    return nullopt;

  pos = MatchInt(query, easting, pos);
  if (pos == string::npos)
    return nullopt;

  pos = MatchInt(query, northing, pos);
  if (pos == string::npos)
    return nullopt;

  return utm_mgrs_utils::UTMtoLatLon(easting, northing, zone_code, zone_letter);
}

// Parse MGRS format "(\d\d\W)\s*(\W\W)\s*(\d+)\s*(\d+)" and converts it to lat,lon.
// Returns true if parsed successfully or false otherwise.
// See utm_mgrs_coords_match_test.cpp for sample MGRS strings
std::optional<ms::LatLon> MatchMGRSCoords(std::string const & query)
{
  long zone_code;
  char zone_letter;
  char square_code[2];
  string eastingStr;
  string northingStr;
  int32_t easting;
  int32_t northing;

  strings::SimpleTokenizer it(query, " \t\r");
  if (!it)
    return nullopt;

  auto token = std::string(*it);
  // Parse 2 digit zone code and 1 char zone letter
  if (token.size() >= 3)
  {
    char dig1 = token[0];
    char dig2 = token[1];
    if (dig1 < '0' || dig1 > '9' || dig2 < '0' || dig2 > '9')
      return nullopt;

    zone_code = (dig1 - '0') * 10 + (dig2 - '0');
    if (zone_code<1 || zone_code > 60)
      return nullopt;

    zone_letter = token[2];
    token = token.substr(3);
  }
  else
    return nullopt;

  // Read next token if needed.
  if (token.size() == 0)
  {
    ++it;
    if (!it)
      return nullopt;
    token = std::string(*it);
  }

  // Parse 2 chars zone code.
  if (token.size() >= 2)
  {
    square_code[0] = token[0];
    square_code[1] = token[1];
    token = token.substr(2);
  }

  // Read next token if needed.
  if (token.size() == 0)
  {
    ++it;
    if (!it)
      return nullopt;
    token = std::string(*it);
  }

  // Parse easting and norhing.
  eastingStr = token;

  // Get next token if available.
  ++it;
  if (it)
    northingStr = std::string(*it);

  // Convert eastingStr & northingStr to numbers.
  if (northingStr.empty())
  {
    // eastingStr contains both easting and northing. Let's split
    if (eastingStr.size()%2 != 0)
      return nullopt;

    int eastingSize = eastingStr.size()/2;
    northingStr = eastingStr.substr(eastingSize);
    eastingStr = eastingStr.substr(0, eastingSize);
  }

  if (eastingStr.size() != northingStr.size() || eastingStr.size()>5 || northingStr.size()>5)
    return nullopt;

  if (!strings::to_int32(eastingStr, easting))
    return nullopt;
  if (eastingStr.size() < 5)
  {
    int decShift = 5 - eastingStr.size();
    easting *= base::PowUint(10, decShift);
  }

  if (!strings::to_int32(northingStr, northing))
    return nullopt;
  if (northingStr.size() < 5)
  {
    int decShift = 5 - northingStr.size();
    northing *= base::PowUint(10, decShift);
  }

  return utm_mgrs_utils::MGRStoLatLon(easting, northing, zone_code, zone_letter, square_code);
}

}
