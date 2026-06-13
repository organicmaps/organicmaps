#pragma once

#include "geometry/latlon.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace irish_grid_utils
{
/* Coordinate systems for the island of Ireland (Northern Ireland and the Republic of Ireland).
 *
 * Two systems are offered, both covering the whole island:
 *  - Irish Grid (IG): a letter-based grid reference, e.g. "O 1516 3447". Defined on the Ireland 1965
 *    datum / Airy Modified 1849 ellipsoid, so converting from the app's WGS84 needs a datum shift
 *    (a 7-parameter Helmert transform, accurate to ~1-2 m, from the OSi/OSNI guide). The high-accuracy
 *    OSi/OSNI gridded transform (the Irish analogue of OSTN15) is omitted to keep the app self-contained.
 *  - Irish Transverse Mercator (ITM): numeric easting/northing, e.g. "715830 734697". Defined on ETRS89,
 *    which equals WGS84 to well under a metre, so no datum shift is needed - just the projection.
 *
 * Both use the same transverse Mercator origin (53°30'N, 8°W) but different ellipsoid, scale and false
 * origin, so IG and ITM are independent numeric values for the same point (ITM is not a fixed offset of IG).
 *
 * References:
 *   https://en.wikipedia.org/wiki/Irish_grid_reference_system
 *   https://en.wikipedia.org/wiki/Irish_Transverse_Mercator
 *   https://www.movable-type.co.uk/scripts/latlong-irish-grid.html
 */

// Digits per axis in an Irish Grid reference; 4 -> 8-figure / 10 m, matching the ~1-2 m datum accuracy.
int constexpr kDefaultFigures = 4;

/* WGS84 lat/lon -> Irish Grid reference, e.g. "O 1516 3447". figures is clamped to [1, 5] (5 -> 1 m).
 * Returns empty outside the Irish Grid lettered area (a 500 km square that also covers the surrounding
 * sea), so a non-empty result does not by itself imply Ireland - callers gate on the region as well
 * (see IsIrishGridRegion) to offer the format only where it is the official one. */
std::string FormatIrishGrid(double lat, double lon, int figures = kDefaultFigures);

/* Parse an Irish Grid reference back to WGS84. Accepts spacing/case variants, e.g. "O 152 345",
 * "O152345", "o 15163 34468". Returns nullopt only if the input is not a well-formed grid reference.
 * A successful parse means a valid in-grid reference, not necessarily an on-land Irish point - the
 * lettered grid extends into the sea and into western Scotland - so callers that need a real Irish
 * location gate on the region (see IsIrishGridRegion). */
std::optional<ms::LatLon> IrishGridToLatLon(std::string_view gridRef);

/* WGS84 lat/lon -> ITM "easting northing" in metres (1 m), e.g. "715830 734697".
 * Returns empty outside a coarse Ireland bounding box (which also reaches western Scotland); that box
 * is just a cheap pre-filter - the region gate is the caller's job. */
std::string FormatITM(double lat, double lon);

/* Parse an ITM "easting northing" pair (space- or comma-separated) back to WGS84. Returns nullopt
 * unless the input is exactly two non-negative integers of at most 7 digits each (ITM values are 6);
 * as with the Irish Grid, geographic validity is the caller's job. */
std::optional<ms::LatLon> ITMToLatLon(std::string_view itm);

/* True if regionId (an Organic Maps mwm region id) is one where the Irish systems are the official
 * reference: Northern Ireland ("UK_Northern Ireland") and the Republic of Ireland, whose mwms are the
 * "Ireland_<province>" leaves (e.g. "Ireland_Leinster"). Shared by both Irish Grid and ITM. */
bool IsIrishGridRegion(std::string_view regionId);
}  // namespace irish_grid_utils
