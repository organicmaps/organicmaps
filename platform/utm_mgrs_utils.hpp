#pragma once

#include <string>

#include "geometry/latlon.hpp"

namespace utm_mgrs_utils
{
// Convert from Lat and Lon coordinates in WGS 84 projection to UTM string
std::optional<std::string> FormatUTM(double lat, double lon);

/* Convert from Lat and Lon coordinates in WGS 84 projection to MGRS square string
 * Parameter prec should be a number from 1 to 5.
 * prec = 1 gives MGRS coordinates "11S PA 7 1" (lowest precision)
 * prec = 2 gives MGRS coordinates "11S PA 72 11"
 * prec = 3 gives MGRS coordinates "11S PA 723 118"
 * prec = 4 gives MGRS coordinates "11S PA 7234 1184"
 * prec = 5 gives MGRS coordinates "11S PA 72349 11844" (highest precision)
 */
std::optional<std::string> FormatMGRS(double lat, double lon, int prec);

// Covevrt UTM coordinates to Lat Lon. If UTM parameters are invalid function returns false
std::optional<ms::LatLon> UTMtoLatLon(double easting, double northing, int zone_code, char zone_letter);

// Covevrt MGRS coordinates to Lat Lon. If parameters are invalid function returns false
std::optional<ms::LatLon> MGRStoLatLon(double easting, double northing, int zone_code, char zone_letter, char square_code[2]);

}  // namespace utm_mgrs_utils
