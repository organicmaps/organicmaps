#include "traffic/speed_groups.hpp"

namespace traffic
{
uint32_t const kSpeedGroupThresholdPercentage[] = {8, 16, 33, 58, 83, 100, 100, 100};

string DebugPrint(SpeedGroup const & group)
{
  switch (group)
  {
  case SpeedGroup::G0: return "G0";
  case SpeedGroup::G1: return "G1";
  case SpeedGroup::G2: return "G2";
  case SpeedGroup::G3: return "G3";
  case SpeedGroup::G4: return "G4";
  case SpeedGroup::G5: return "G5";
  case SpeedGroup::TempBlock: return "TempBlock";
  case SpeedGroup::Unknown: return "Unknown";
  case SpeedGroup::Count: return "Count";
  }
}
}  // namespace traffic
