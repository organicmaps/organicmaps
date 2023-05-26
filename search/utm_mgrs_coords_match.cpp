#include "search/utm_mgrs_coords_match.hpp"

#include "base/string_utils.hpp"

#include "platform/utm_mgrs_utils.hpp"

#include <iterator>
#include <string>
#include <utility>
#include <iostream>
#include <math.h>

using namespace std;

namespace search
{

bool IsSpace(char ch);
string::size_type FindNonSpace(string const & txt, string::size_type startPos);
string::size_type MatchZoneCode(string const & query, int &zone_code);
string::size_type MatchZoneLetter(string const & query, char &zone_letter, string::size_type startPos);
string::size_type MatchLong(string const & query, long &value, string::size_type startPos);
bool ParseFixDigitsNumber(string const & txt, int numDigits, string::size_type startPos, long & value);
std::string StripSpaces(string const & txt);
bool parseLong(string const & txt, long & value);

// Parse UTM format "(\d\d)\s?(\W)\s+(\d+)\s+(\d+)" and converts it to lat,lon.
// Return true if parsed successfully or false otherwise.
// See utm_mgrs_coords_match_test.cpp for sample UTM strings
bool MatchUTMCoords(string const & query, double & lat, double & lon)
{
    int zone_code;
    char zone_letter;
    long easting, northing;

    string::size_type pos = MatchZoneCode(query, zone_code);
    if (pos == string::npos)
        return false;

    pos = MatchZoneLetter(query, zone_letter, pos);
    if (pos == string::npos)
        return false;

    pos = MatchLong(query, easting, pos);
    if (pos == string::npos)
        return false;

    pos = MatchLong(query, northing, pos);
    if (pos == string::npos)
        return false;

    return utm_mgrs_utils::UTMtoLatLon((double)easting, (double)northing, zone_code, zone_letter, lat, lon);
}

// Matches 2 digits zone code. Returns end position of matched chars or string::npos if no match.
string::size_type MatchZoneCode(string const & query, int &zone_code)
{
    auto pos = FindNonSpace(query, 0);
    if (query.size()-pos < 2)
        return string::npos;

    char dig1 = query[pos];
    char dig2 = query[pos+1];
    if (dig1 < '0' || dig1 > '9' || dig2 < '0' || dig2 > '9')
        return string::npos;

    zone_code = (dig1 - '0') * 10 + (dig2 - '0');
    return pos + 2;
}


// Matches zone letter ignoring spaces. Returns end position of matched chars or string::npos if no match.
string::size_type MatchZoneLetter(string const & query, char &zone_letter, string::size_type startPos)
{
    auto pos = FindNonSpace(query, startPos);
    if (query.size() == pos)
        return string::npos;

    char l = query[pos];
    if (l < 'A' || l > 'Z')
        return string::npos;

    zone_letter = l;
    return pos + 1;
}

// Matches long number ignoring spaces. Returns end position of matched chars or string::npos if no match.
string::size_type MatchLong(string const & query, long &value, string::size_type startPos)
{
    auto pos = FindNonSpace(query, startPos);
    if (query.size() == pos)
        return string::npos;

    long n = 0;
    while(pos < query.size())
    {
        char ch = query[pos];
        if (ch >= '0' && ch <= '9')
        {
            n = n*10 + (ch-'0');
            pos ++;
        }
        else if (IsSpace(ch))
            break;
        else
            return string::npos;
    }

    value = n;
    return pos;
}

// Return position of non-space char starting from startPos.
// Return txt.size() if no such character found.
string::size_type FindNonSpace(string const & txt, string::size_type startPos)
{
    auto pos = startPos;
    while(pos < txt.size())
    {
        if (IsSpace(txt[pos]))
            pos++;
        else
            break;
    }

    return pos;
}

bool IsSpace(char ch)
{
    return ch == ' ' || ch == '\t' || ch == '\r';
}

// Parse MGRS format "(\d\d\W)\s*(\W\W)\s*(\d+)\s*(\d+)" and converts it to lat,lon.
// Returns true if parsed successfully or false otherwise.
// See utm_mgrs_coords_match_test.cpp for sample MGRS strings
bool MatchMGRSCoords(std::string const & query, double & lat, double & lon)
{
    long zone_code;
    char zone_letter;
    char square_code[2];
    string eastingStr;
    string northingStr;
    long easting;
    long northing;

    strings::SimpleTokenizer it(query, " \t\r");
    if (!it)
        return false;

    auto token = std::string(*it);
    // Parse 2 digit zone code and 1 char zone letter
    if (token.size() >= 3)
    {
        char dig1 = token[0];
        char dig2 = token[1];
        if (dig1 < '0' || dig1 > '9' || dig2 < '0' || dig2 > '9')
            return false;

        zone_code = (dig1 - '0') * 10 + (dig2 - '0');
        if (zone_code<1 || zone_code > 60)
            return false;

        zone_letter = token[2];
        token = token.substr(3);
    }
    else
        return false;

    // Read next token if needed.
    if (token.size() == 0)
    {
        ++it;
        if (!it)
            return false;
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
            return false;
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
            return false;

        int eastingSize = eastingStr.size()/2;
        northingStr = eastingStr.substr(eastingSize);
        eastingStr = eastingStr.substr(0, eastingSize);
    }

    if (eastingStr.size() != northingStr.size() || eastingStr.size()>5 || northingStr.size()>5)
        return false;

    if (!parseLong(eastingStr, easting))
        return false;
    if (eastingStr.size() < 5)
    {
        int decShift = 5 - eastingStr.size();
        easting *= pow(10L, decShift);
    }

    if (!parseLong(northingStr, northing))
        return false;
    if (northingStr.size() < 5)
    {
        int decShift = 5 - northingStr.size();
        northing *= pow(10L, decShift);
    }

    return utm_mgrs_utils::MGRStoLatLon((double)easting, (double)northing, zone_code, zone_letter, square_code, lat, lon);
}

bool parseLong(string const & txt, long & value)
{
    long parsedValue = 0;
    for (char ch : txt)
    {
        if (ch < '0' || ch > '9')
            return false;
        parsedValue = parsedValue * 10 + (ch - '0');
    }
    value = parsedValue;
    return true;
}

}
