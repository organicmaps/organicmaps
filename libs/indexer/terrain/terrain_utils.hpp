#pragma once

#include "platform/measurement_utils.hpp"

#include "base/assert.hpp"

#include "defines.hpp"

#include <cmath>
#include <string>

namespace terrain
{
// The block file name for its bottom-left corner, e.g. "N45E006.twm" or "S33W072.twm"
// (the SRTM tile naming, must match SrtmTile::GetBase of the generator).
inline std::string GetBlockFileName(int bottomLat, int leftLon)
{
  ASSERT(bottomLat >= -90 && bottomLat < 90 && leftLon >= -180 && leftLon < 180, (bottomLat, leftLon));
  char buffer[16];
  std::snprintf(buffer, sizeof(buffer), "%c%02d%c%03d" TERRAIN_FILE_EXT, bottomLat < 0 ? 'S' : 'N', std::abs(bottomLat),
                leftLon < 0 ? 'W' : 'E', std::abs(leftLon));
  return buffer;
}

// The isolines step for the draw zoom level, in meters or feet per the measurement units
// (cf. the maplibre-contour intervals; the feet steps are the USGS-style round values).
// The steps must be multiples of 10 to keep the isoline style class mapping working, and
// the traced levels land on the style classes with per-class zoom visibility gates (see
// kAltClasses in RuleDrawer::DrawDynamicIsolines): e.g. the metric 50 m trace at z12-13
// renders only the 100 m lines until the step_50 class opens. The feet values are chosen
// so the VISIBLE density stays close to the metric one through those gates: 200 ft maps
// to the step_100/500/1000 classes only, mirroring the metric 100 m ladder.
inline int32_t GetIsolinesStepForZoom(int zoom, measurement_utils::Units units)
{
  if (units == measurement_utils::Units::Imperial)
    return zoom <= 14 ? 200 : 20;
  if (zoom <= 11)
    return 100;
  if (zoom <= 13)
    return 50;
  if (zoom == 14)
    return 20;
  return 10;
}

// The altitude labels step for the draw zoom and the tile's traced altitudes range
// (both in the units), 0 = no labels. Must be a multiple of the corresponding
// GetIsolinesStepForZoom, so the labeled levels exist among the traced ones.
inline int32_t GetIsolinesLabelStepForZoom(int zoom, int32_t altitudeRange, measurement_utils::Units units)
{
  if (units == measurement_utils::Units::Imperial)
  {
    if (zoom <= 11)
      return 1000;
    if (zoom <= 13)
      return altitudeRange > 3000 ? 1000 : 200;
    if (zoom == 14)
      return 200;
    return altitudeRange > 600 ? 100 : 40;
  }
  if (zoom <= 11)
    return 500;
  if (zoom <= 13)
    return altitudeRange > 1000 ? 500 : 100;
  if (zoom == 14)
    return 100;
  return altitudeRange > 200 ? 50 : 20;
}
}  // namespace terrain
