#pragma once

#include "base/assert.hpp"

#include <cstdint>
#include <string>

namespace df
{
// Analytic area patterns. A hatch is drawn over the area fill as a separate transparent overlay; a solid-fill pattern
// modulates the fill color in place.
enum class AreaPattern : uint8_t
{
  None,
  Hatch45d,
  HatchDash,
  Stipple,
  Speckle,
  Grid,
  Forest,
};

inline std::string DebugPrint(AreaPattern pattern)
{
  switch (pattern)
  {
  case AreaPattern::None: return "None";
  case AreaPattern::Hatch45d: return "Hatch45d";
  case AreaPattern::HatchDash: return "HatchDash";
  case AreaPattern::Stipple: return "Stipple";
  case AreaPattern::Speckle: return "Speckle";
  case AreaPattern::Grid: return "Grid";
  case AreaPattern::Forest: return "Forest";
  }
  UNREACHABLE();
}
}  // namespace df
