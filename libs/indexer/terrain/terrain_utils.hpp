#pragma once

#include "indexer/drules_struct.hpp"

#include "platform/measurement_utils.hpp"

#include "base/assert.hpp"

#include "defines.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <string>

namespace terrain
{
// The first zoom of the terrain layers (the isoline style classes open here, see
// data/styles); shared by the drape gate and the isolines availability hint.
int constexpr kMinIsolinesZoom = 11;

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

// Resolves the dynamic isolines drawing policy from the current map style for one draw
// zoom: the altitude step to trace and the line/label drules of every traced altitude.
// The style is the single source of the steps: a class the style opens at some zoom
// (e.g. line|z11-[isoline=step_1000]) traces and draws from exactly that zoom, and a
// level is labeled iff its class has a visible pathtext rule. The altitude -> class
// mapping matches the baked isoline features (see generator isolines_generator); the
// sea depths (negative altitudes) follow the same ladder inverted: -N draws as N does.
// Constructed once per tile draw (see RuleDrawer::DrawDynamicIsolines), the per-isoline
// lookups are trivial.
// TODO(terrain): correct the resolved steps by the tile relief or coordinates, e.g.
// shift the class visibility a zoom earlier for manually highlighted regions.
class IsolinesStyle
{
public:
  IsolinesStyle(int zoom, measurement_utils::Units units);

  // The trace step in the display units: the finest class rung with a visible line
  // drule, 0 when the style draws no isolines at this zoom.
  int32_t GetStep() const { return m_step; }

  // The line drule of the altitude's class, nullptr when invisible at this zoom.
  drule::LineRule const * GetLineRule(int32_t altitude) const { return GetClass(altitude).m_line; }

  // The label drule of the altitude's class (with the primary caption always present),
  // nullptr when the style does not label this altitude at this zoom.
  drule::PathTextRule const * GetPathTextRule(int32_t altitude) const { return GetClass(altitude).m_text; }

private:
  struct ClassStyle
  {
    int32_t m_rung = 0;
    drule::LineRule const * m_line = nullptr;
    drule::PathTextRule const * m_text = nullptr;
  };

  ClassStyle const & GetClass(int32_t altitude) const;

  std::array<ClassStyle, 5> m_classes;  // Coarse to fine, see kClassDefs in the cpp.
  ClassStyle m_zero;                    // The zero altitude line, a style class of its own.
  int32_t m_step = 0;
};
}  // namespace terrain
