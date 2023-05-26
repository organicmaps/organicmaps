#include "platform/utm_mgrs_utils.hpp"

#include "base/string_utils.hpp"

#include <string>
#include <iostream>
#include <cmath>

namespace utm_mgrs_utils
{
using namespace std;

typedef struct UTMPoint_Value
{
    double easting;
    double northing;
    int zone_number;
    char zone_letter;

} UTMPoint;

UTMPoint latlon_to_utm(double lat, double lon);
int latlon_to_zone_number(double lat, double lon);
char latitude_to_zone_letter(double lat);
int zone_number_to_central_longitude(int zone_number);
double mod_angle(double value);

string utm_to_str(UTMPoint point);
string utm_to_mgrs_str(UTMPoint point, int precision);
int zone_to_100k(int i);
string get_100k_id(double easting, double northing, int zone_number);
double square_char_to_easting(char e, int set);
double square_char_to_northing(char n, int set);
double zone_to_min_northing(char zoneLetter);

const double PI = 3.14159165358979;
const double K0 = 0.9996;
const double E = 0.00669438;
const double E2 = E * E;
const double E3 = E2 * E;
const double E_P2 = E / (1 - E);
const double R = 6378137.0;

const double SQRT_E = sqrt(1 - E);
const double _E = (1 - SQRT_E) / (1 + SQRT_E);
const double _E2 = _E * _E;
const double _E3 = _E2 * _E;
const double _E4 = _E3 * _E;
const double _E5 = _E4 * _E;

const double M1 = (1.0 - E / 4.0 - 3.0 * E2 / 64.0 - 5.0 * E3 / 256.0);
const double M2 = (3.0 * E / 8.0 + 3.0 * E2 / 32.0 + 45.0 * E3 / 1024.0);
const double M3 = (15.0 * E2 / 256.0 + 45.0 * E3 / 1024.0);
const double M4 = (35.0 * E3 / 3072.0);

const double P2 = (3.0 / 2.0 * _E - 27.0 / 32.0 * _E3 + 269.0 / 512.0 * _E5);
const double P3 = (21.0 / 16.0 * _E2 - 55.0 / 32.0 * _E4);
const double P4 = (151.0 / 96.0 * _E3 - 417.0 / 128.0 * _E5);
const double P5 = (1097.0 / 512.0 * _E4);

const string ZONE_LETTERS = "CDEFGHJKLMNPQRSTUVWXX";
double const DEGREES_TO_RADIANS = 0.017453292519943295;
double const RADIANS_TO_DEGREES = 57.29577951308232;

const int NUM_100K_SETS = 6;
const int SET_ORIGIN_COLUMN_LETTERS[] = { 'A', 'J', 'S', 'A', 'J', 'S' };
const int SET_ORIGIN_ROW_LETTERS[] = { 'A', 'F', 'A', 'F', 'A', 'F' };


double degToRad(double deg)
{
    return deg * DEGREES_TO_RADIANS;
}

double radToDeg(double rad)
{
    return rad * RADIANS_TO_DEGREES;
}

// Convert lat,lon for WSG 84 ellipsoid to MGRS string.
string FormatMGRS(double lat, double lon, int precision)
{
    if (precision > 5)
        precision = 5;
    else if (precision < 1)
        precision = 1;

    UTMPoint mgrsp = latlon_to_utm(lat, lon);

    // Need to add this to set the right letter for the latitude.
    mgrsp.zone_letter = latitude_to_zone_letter(lat);
    return utm_to_mgrs_str(mgrsp, precision);
}

// Convert lat,lon for WSG 84 ellipsoid to UTM string.
string FormatUTM(double lat, double lon)
{
    UTMPoint utm = latlon_to_utm(lat, lon);
    return utm_to_str(utm);
}

// Main algorithm. Formulars sourcre: https://github.com/Turbo87/utm
UTMPoint latlon_to_utm(double lat, double lon)
{
    double lat_rad = degToRad(lat);
    double lat_sin = sin(lat_rad);
    double lat_cos = cos(lat_rad);

    double lat_tan = lat_sin / lat_cos;
    double lat_tan2 = lat_tan * lat_tan;
    double lat_tan4 = lat_tan2 * lat_tan2;

    int zone_number = latlon_to_zone_number(lat, lon);
    char zone_letter = (lat >= 0.0) ? 'N' : 'S';

    double lon_rad = degToRad(lon);
    double central_lon = zone_number_to_central_longitude(zone_number);
    double central_lon_rad = degToRad(central_lon);

    double n = R / sqrt(1 - E * lat_sin * lat_sin);
    double c = E_P2 * lat_cos * lat_cos;

    double a = lat_cos * mod_angle(lon_rad - central_lon_rad);
    double a2 = a * a;
    double a3 = a2 * a;
    double a4 = a3 * a;
    double a5 = a4 * a;
    double a6 = a5 * a;

    double m = R * (M1 * lat_rad -
             M2 * sin(2 * lat_rad) +
             M3 * sin(4 * lat_rad) -
             M4 * sin(6 * lat_rad));

    double easting = K0 * n * (a +
                        a3 / 6 * (1 - lat_tan2 + c) +
                        a5 / 120 * (5 - 18 * lat_tan2 + lat_tan4 + 72 * c - 58 * E_P2)) + 500000.0;

    double northing = K0 * (m + n * lat_tan * (a2 / 2 +
                                        a4 / 24 * (5 - lat_tan2 + 9 * c + 4 * c * c) +
                                        a6 / 720 * (61 - 58 * lat_tan2 + lat_tan4 + 600 * c - 330 * E_P2)));
    if (lat < 0.0)
        northing += 10000000.0;

    return {easting, northing, zone_number, zone_letter};

}

int latlon_to_zone_number(double lat, double lon)
{
    if (56.0 <= lat && lat < 64.0 && 3.0 <= lon && lon < 12.0)
        return 32;

    if (72.0 <= lat && lat <= 84.0 and lon >= 0.0)
    {
        if (lon < 9.0)
            return 31;
        else if (lon < 21.0)
            return 33;
        else if (lon < 33.0)
            return 35;
        else if (lon < 42.0)
            return 37;
    }

    return int((lon + 180.0) / 6.0) + 1;
}

char latitude_to_zone_letter(double lat)
{
    if (-80.0 <= lat && lat <= 84.0)
        return ZONE_LETTERS[int(lat + 80.0) >> 3];
    else
        return '?';
}

int zone_number_to_central_longitude(int zone_number)
{
    return (zone_number - 1) * 6 - 180 + 3;
}

// Returns angle in radians to be between -PI and PI.
double mod_angle(double value)
{
    if (value < -PI)
        value += - 2 * PI * ((int)(value - PI) / (2 * PI));
    else if (value > PI)
        value -= 2 * PI * ((int)(value + PI) / (2 * PI));
    return value;
}

// Generate UTM string from UTM point parameters.
string utm_to_str(UTMPoint point)
{
    return to_string(point.zone_number) + string(1, point.zone_letter) + " " +\
            to_string(int(round(point.easting))) + " " + \
            to_string(int(round(point.northing)));
}

// Convert UTM point parameters to MGRS parameters. Additional 2 char code is deducted.
// Easting and northing parameters are reduced to 5 digits.
string utm_to_mgrs_str(UTMPoint point, int precision)
{
    if (point.zone_letter == 'Z') {
        return "Latitude limit exceeded";
    } else {
        string eastingStr = strings::to_string_prec(point.easting, precision+1);
        string northingStr = strings::to_string_prec(point.northing, precision+1);

        if (northingStr.size() > 6)
            northingStr = northingStr.substr(northingStr.size() - 6);

        return strings::to_string_prec((long) point.zone_number, 2) + point.zone_letter + " " + \
                get_100k_id(point.easting, point.northing, point.zone_number) + " " + \
                eastingStr.substr(1, precision) + " " + \
                northingStr.substr(1, precision);
    }
}

// Build 2 chars string with 100k square ID
string get_100k_id(double easting, double northing, int zone_number)
{
    int set = zone_to_100k(zone_number);
    int setColumn = ((int) easting / 100000);
    int setRow = ((int) northing / 100000) % 20;

    int colOrigin = SET_ORIGIN_COLUMN_LETTERS[set - 1];
    int rowOrigin = SET_ORIGIN_ROW_LETTERS[set - 1];

    int colInt = colOrigin + setColumn - 1;
    int rowInt = rowOrigin + setRow;
    bool rollover = false;

    if (colInt > 'Z') {
        colInt = colInt - 'Z' + 'A' - 1;
        rollover = true;
    }

    if (colInt == 'I' || (colOrigin < 'I' && colInt > 'I') || ((colInt > 'I' || colOrigin < 'I') && rollover)) {
        colInt++;
    }
    if (colInt == 'O' || (colOrigin < 'O' && colInt > 'O') || ((colInt > 'O' || colOrigin < 'O') && rollover)) {
        colInt++;
        if (colInt == 'I') {
            colInt++;
        }
    }

    if (colInt > 'Z') {
        colInt = colInt - 'Z' + 'A' - 1;
    }

    if (rowInt > 'V') {
        rowInt = rowInt - 'V' + 'A' - 1;
        rollover = true;
    } else {
        rollover = false;
    }

    if (rowInt == 'I' || (rowOrigin < 'I' && rowInt > 'I') || ((rowInt > 'I' || rowOrigin < 'I') && rollover)) {
        rowInt++;
    }

    if (rowInt == 'O' || (rowOrigin < 'O' && rowInt > 'O') || ((rowInt > 'O' || rowOrigin < 'O') && rollover)) {
        rowInt++;
        if (rowInt == 'I') {
            rowInt++;
        }
    }

    if (rowInt > 'V') {
        rowInt = rowInt - 'V' + 'A' - 1;
    }

    string twoLetter = {char(colInt), char(rowInt)};

    return twoLetter;
}

int zone_to_100k(int i) {
    int set = i % NUM_100K_SETS;
    if (set == 0)
        set = NUM_100K_SETS;
    return set;
}

// Convert UTM parameters to lat,lon for WSG 84 ellipsoid.
// If UTM parameters are valid lat and lon references are used to output calculated coordinates.
// Otherwise function returns 'false'.
bool UTMtoLatLon(double easting, double northing, int zone_number, char zone_letter, double &lat, double &lon)
{
    if (zone_number < 1 || zone_number > 60)
        return false;

    if (easting < 100000.0 || easting >= 1000000.0)
        return false;

    if (northing < 0.0 || northing > 10000000.0)
        return false;

    if (zone_letter<'C' || zone_letter>'X' || zone_letter == 'I' || zone_letter == 'O')
        return false;

    bool northern = (zone_letter >= 'N');
    double x = easting - 500000.0;
    double y = northing;

    if (!northern)
        y -= 10000000.0;

    double m = y / K0;
    double mu = m / (R * M1);

    double p_rad = (mu +
             P2 * sin(2.0 * mu) +
             P3 * sin(4.0 * mu) +
             P4 * sin(6.0 * mu) +
             P5 * sin(8.0 * mu));

    double p_sin = sin(p_rad);
    double p_sin2 = p_sin * p_sin;

    double p_cos = cos(p_rad);

    double p_tan = p_sin / p_cos;
    double p_tan2 = p_tan * p_tan;
    double p_tan4 = p_tan2 * p_tan2;

    double ep_sin = 1 - E * p_sin2;
    double ep_sin_sqrt = sqrt(1 - E * p_sin2);

    double n = R / ep_sin_sqrt;
    double r = (1 - E) / ep_sin;

    double c = E_P2 * p_cos * p_cos;
    double c2 = c * c;

    double d = x / (n * K0);
    double d2 = d * d;
    double d3 = d2 * d;
    double d4 = d3 * d;
    double d5 = d4 * d;
    double d6 = d5 * d;

    double latitude = (p_rad - (p_tan / r) *
                (d2 / 2.0 -
                 d4 / 24.0 * (5.0 + 3.0 * p_tan2 + 10.0 * c - 4.0 * c2 - 9.0 * E_P2)) +
                 d6 / 720.0 * (61.0 + 90.0 * p_tan2 + 298.0 * c + 45.0 * p_tan4 - 252.0 * E_P2 - 3.0 * c2));

    double longitude = (d -
                 d3 / 6.0 * (1.0 + 2.0 * p_tan2 + c) +
                 d5 / 120.0 * (5.0 - 2.0 * c + 28.0 * p_tan2 - 3.0 * c2 + 8.0 * E_P2 + 24.0 * p_tan4)) / p_cos;

    longitude = mod_angle(longitude + degToRad(zone_number_to_central_longitude(zone_number)));

    lat = radToDeg(latitude);
    lon = radToDeg(longitude);

    return true;
}

// Convert MGRS parameters to UTM parameters and then use UTM to lat,lon conversion.
bool MGRStoLatLon(double easting, double northing, int zone_code, char zone_letter, char square_code[2], double &lat, double &lon)
{
    // Convert easting and northing according to zone_code and square_code
    if (zone_code < 1 || zone_code > 60)
        return false;

    if (zone_letter <= 'B' || zone_letter >= 'Y' || zone_letter == 'I' || zone_letter == 'O')
        return false;

    int set = zone_to_100k(zone_code);

    char char1 = square_code[0];
    char char2 = square_code[1];

    if (char1 < 'A' || char2 < 'A' || char1 > 'Z' || char2 > 'Z' || char1 == 'I' || char2 == 'I' || char1 == 'O' || char2 == 'O')
        return false;

    float east100k = square_char_to_easting(char1, set);
    if (east100k < 0)
        return false;

    float north100k = square_char_to_northing(char2, set);
    if (north100k < 0)
        return false;

    double minNorthing = zone_to_min_northing(zone_letter);
    if (minNorthing < 0)
        return false;

    while (north100k < minNorthing)
        north100k += 2000000.0;

    easting  += east100k;
    northing += north100k;

    return UTMtoLatLon(easting, northing, zone_code, zone_letter, lat, lon);
}

/**
 * Given the first letter from a two-letter MGRS 100k zone, and given the
 * MGRS table set for the zone number, figure out the easting value that
 * should be added to the other, secondary easting value.
 */
double square_char_to_easting(char e, int set) {
    int curCol = SET_ORIGIN_COLUMN_LETTERS[set - 1];
    double eastingValue = 100000.0;
    bool rewindMarker = false;

    while (curCol != e)
    {
        curCol++;
        if (curCol == 'I')
            curCol++;
        if (curCol == 'O')
            curCol++;
        if (curCol > 'Z')
        {
            if (rewindMarker)
                return -1;
            curCol = 'A';
            rewindMarker = true;
        }
        eastingValue += 100000.0;
    }

    return eastingValue;
}

/**
 * Given the second letter from a two-letter MGRS 100k zone, and given the
 * MGRS table set for the zone number, figure out the northing value that
 * should be added to the other, secondary northing value. You have to
 * remember that Northings are determined from the equator, and the vertical
 * cycle of letters mean a 2000000 additional northing meters. This happens
 * approx. every 18 degrees of latitude. This method does *NOT* count any
 * additional northings. You have to figure out how many 2000000 meters need
 * to be added for the zone letter of the MGRS coordinate.
 *
 * @param n
 *            second letter of the MGRS 100k zone
 * @param set
 *            the MGRS table set number, which is dependent on the UTM zone
 *            number.
 */
double square_char_to_northing(char n, int set) {

    if (n > 'V')
        return -1;

    int curRow = SET_ORIGIN_ROW_LETTERS[set - 1];
    double northingValue = 0.0;
    bool rewindMarker = false;

    while (curRow != n)
    {
        curRow++;
        if (curRow == 'I')
            curRow++;
        if (curRow == 'O')
            curRow++;
        // fixing a bug making whole application hang in this loop
        // when 'n' is a wrong character
        if (curRow > 'V') {
            if (rewindMarker) // making sure that this loop ends
                return -1;
            curRow = 'A';
            rewindMarker = true;
        }
        northingValue += 100000.0;
    }

    return northingValue;
}

/**
 * The function getMinNorthing returns the minimum northing value of a MGRS
 * zone.
 *
 * portted from Geotrans' c Latitude_Band_Value structure table. zoneLetter
 * : MGRS zone (input)
 */
double zone_to_min_northing(char zoneLetter)
{
    double northing;
    switch (zoneLetter)
    {
        case 'C':
            northing = 1100000.0;
            break;
        case 'D':
            northing = 2000000.0;
            break;
        case 'E':
            northing = 2800000.0;
            break;
        case 'F':
            northing = 3700000.0;
            break;
        case 'G':
            northing = 4600000.0;
            break;
        case 'H':
            northing = 5500000.0;
            break;
        case 'J':
            northing = 6400000.0;
            break;
        case 'K':
            northing = 7300000.0;
            break;
        case 'L':
            northing = 8200000.0;
            break;
        case 'M':
            northing = 9100000.0;
            break;
        case 'N':
            northing = 0.0;
            break;
        case 'P':
            northing = 800000.0;
            break;
        case 'Q':
            northing = 1700000.0;
            break;
        case 'R':
            northing = 2600000.0;
            break;
        case 'S':
            northing = 3500000.0;
            break;
        case 'T':
            northing = 4400000.0;
            break;
        case 'U':
            northing = 5300000.0;
            break;
        case 'V':
            northing = 6200000.0;
            break;
        case 'W':
            northing = 7000000.0;
            break;
        case 'X':
            northing = 7900000.0;
            break;
        default:
            northing = -1.0;
    }

    return northing;
}


}  // namespace utm_mgrs_utils
