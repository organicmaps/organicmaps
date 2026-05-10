#include "traffic/speed_groups.hpp"

#include "base/math.hpp"

namespace traffic
{
uint32_t const kSpeedGroupThresholdPercentage[] = {8, 16, 33, 58, 83, 100, 100, 100};

SpeedGroup GetSpeedGroupByPercentage(double p)
{
  p = math::Clamp(p, 0.0, 100.0);
  SpeedGroup res = SpeedGroup::Unknown;
  for (int i = static_cast<int>(SpeedGroup::Count) - 1; i >= 0; --i)
    if (p <= kSpeedGroupThresholdPercentage[i])
      res = static_cast<SpeedGroup>(i);
  return res;
}

std::string_view DebugPrint(SpeedGroup group)
{
  switch (group)
  {
    using enum SpeedGroup;
  case G0: return "G0";
  case G1: return "G1";
  case G2: return "G2";
  case G3: return "G3";
  case G4: return "G4";
  case G5: return "G5";
  case TempBlock: return "TempBlock";
  case Unknown: return "Unknown";
  case Count: return "Count";
  }
  UNREACHABLE();
}
}  // namespace traffic
