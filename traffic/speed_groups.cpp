#include "traffic/speed_groups.hpp"

#include "base/math.hpp"

namespace traffic
{
uint32_t const kSpeedGroupThresholdPercentage[] = {8, 16, 33, 58, 83, 100, 100, 100};

SpeedGroup GetSpeedGroupByPercentage(double p)
{
  p = base::Clamp(p, 0.0, 100.0);
  SpeedGroup res = SpeedGroup::Unknown;
  for (int i = static_cast<int>(SpeedGroup::Count) - 1; i >= 0; --i)
  {
    if (p <= kSpeedGroupThresholdPercentage[i])
      res = static_cast<SpeedGroup>(i);
  }
  return res;
}
}  // namespace traffic
