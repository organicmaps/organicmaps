#pragma once

#include "geometry/point_with_altitude.hpp"

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

// The isolines step in meters for the draw zoom level (cf. the maplibre-contour intervals).
inline geometry::Altitude GetIsolinesStepForZoom(int zoom)
{
  if (zoom <= 11)
    return 100;
  if (zoom <= 13)
    return 50;
  if (zoom == 14)
    return 20;
  return 10;
}

// The altitude labels step for the draw zoom and the tile's traced altitudes range, 0 = no labels.
inline geometry::Altitude GetIsolinesLabelStepForZoom(int zoom, geometry::Altitude altitudeRange)
{
  if (zoom <= 11)
    return 500;
  if (zoom <= 13)
    return altitudeRange > 1000 ? 500 : 100;
  if (zoom == 14)
    return 100;
  return altitudeRange > 200 ? 50 : 20;
}
}  // namespace terrain
