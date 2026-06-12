#pragma once

#include "geometry/latlon.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace os_grid_utils
{
/* British National Grid (Ordnance Survey OS Grid) references for Great Britain.
 *
 * Unlike UTM/MGRS (which are defined on WGS84 and can be projected directly),
 * the OS National Grid is defined on the OSGB36 datum / Airy 1830 ellipsoid.
 * Converting from the app's WGS84 coordinates therefore requires a datum shift
 * (a 7-parameter Helmert transformation, accurate to ~3-5 m across GB) before
 * the transverse Mercator projection. OSTN15 would give ~0.1 m but needs a
 * multi-megabyte shift grid; the Helmert transform is enough for grid
 * references and keeps the app self-contained.
 *
 * References:
 *   https://en.wikipedia.org/wiki/Ordnance_Survey_National_Grid
 *   https://www.movable-type.co.uk/scripts/latlong-os-gridref.html
 *   "A guide to coordinate systems in Great Britain" (Ordnance Survey)
 */

// Number of digits per axis in a grid reference; 4 -> 8-figure / 10 m resolution,
// which matches the ~5 m accuracy of the Helmert datum transform.
int constexpr kDefaultFigures = 4;

/* Convert WGS84 lat/lon to an OS Grid reference string, e.g. "SW 7400 4210".
 * figures is clamped to [1, 5] (5 -> 10-figure / 1 m).
 * Returns an empty string outside the National Grid rectangle. That rectangle also
 * covers Northern Ireland, the Republic of Ireland and the surrounding sea, so a
 * non-empty result alone does not imply Great Britain - callers additionally gate on
 * the region (see IsOSGridRegion) to offer the format only where it is the official one.
 */
std::string FormatOSGrid(double lat, double lon, int figures = kDefaultFigures);

/* Parse an OS Grid reference back to WGS84 lat/lon. Accepts spacing and case
 * variants, e.g. "SW 740 421", "SW740421", "SW 74000 42100".
 * Returns nullopt if the input is not a valid grid reference.
 */
std::optional<ms::LatLon> OSGridToLatLon(std::string_view gridRef);

/* True if regionId (an Organic Maps mwm region id) is covered by the British National
 * Grid: Great Britain (England, Scotland, Wales) and the Isle of Man. Northern Ireland
 * and the Republic of Ireland are excluded - they use the Irish Grid, even though the
 * projection rectangle mathematically reaches them. Used to offer the OS Grid format
 * only where it is the official reference.
 */
bool IsOSGridRegion(std::string_view regionId);
}  // namespace os_grid_utils
