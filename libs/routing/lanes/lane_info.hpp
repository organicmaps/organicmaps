#pragma once

#include "routing/lanes/lane_way.hpp"

#include <vector>

namespace routing::turns::lanes
{
struct LaneInfo
{
  LaneWays laneWays;
  LaneWay recommendedWay = LaneWay::None;

  bool operator==(LaneInfo const & rhs) const
  {
    return laneWays == rhs.laneWays && recommendedWay == rhs.recommendedWay;
  }
};
using LanesInfo = std::vector<LaneInfo>;

std::string DebugPrint(LaneInfo const & laneInfo);
std::string DebugPrint(LanesInfo const & lanesInfo);
}  // namespace routing::turns::lanes
