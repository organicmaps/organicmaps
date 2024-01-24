#pragma once

#include <optional>
#include <string>

#include "geometry/latlon.hpp"

namespace utm_mgrs_utils
{
/* Convert from Lat and Lon coordinates in WGS 84 projection to UTM string.
 * Preconditions:
 *   -180 < lon <= 180
 *   -80 < lat <= 84
 *
 * TODO: use Universal polar stereographic coordinate system for North pole
 *       and South pole regions. Would be needed when OM shows 3D globe.
 */
std::string FormatUTM(double lat, double lon);

/* Convert from Lat and Lon coordinates in WGS 84 projection to MGRS square string
 * Parameter prec should be a number from 1 to 5.
 * prec = 1 gives MGRS coordinates "11S PA 7 1" (lowest precision)
 * prec = 2 gives MGRS coordinates "11S PA 72 11"
 * prec = 3 gives MGRS coordinates "11S PA 723 118"
 * prec = 4 gives MGRS coordinates "11S PA 7234 1184"
 * prec = 5 gives MGRS coordinates "11S PA 72349 11844" (highest precision)
 *
 * Preconditions:
 *   -180 < lon <= 180
 *   -80 < lat <= 84
 *
 * TODO: use Universal polar stereographic coordinate system for North pole
 *       and South pole regions. Would be needed when OM shows 3D globe.
 */
std::string FormatMGRS(double lat, double lon, int prec);

// Covevrt UTM coordinates to Lat Lon. If UTM parameters are invalid function returns false
std::optional<ms::LatLon> UTMtoLatLon(int easting, int northing, int zone_code, char zone_letter);

// Covevrt MGRS coordinates to Lat Lon. If parameters are invalid function returns false
std::optional<ms::LatLon> MGRStoLatLon(int easting, int northing, int zoneCode, char zone_letter, char squareCode[2]);

}  // namespace utm_mgrs_utils
