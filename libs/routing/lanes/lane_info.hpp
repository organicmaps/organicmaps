#pragma once

#include "routing/lanes/lane_way.hpp"

#include <cstdint>
#include <vector>

namespace routing::turns::lanes
{
struct LaneInfo
{
  LaneWays laneWays;
  LaneWay recommendedWay = LaneWay::None;
  /// How many adjacent identical physical lanes this entry stands for. Always 1 as parsed;
  /// CollapseLanes() merges runs of identical non-recommended lanes (toll plazas reach ~50
  /// lanes) and stores the run length here so UIs can render one icon with a count badge.
  std::uint16_t similarLanesCount = 1;

  bool operator==(LaneInfo const & rhs) const
  {
    return laneWays == rhs.laneWays && recommendedWay == rhs.recommendedWay &&
           similarLanesCount == rhs.similarLanesCount;
  }
};
using LanesInfo = std::vector<LaneInfo>;

std::string DebugPrint(LaneInfo const & laneInfo);
std::string DebugPrint(LanesInfo const & lanesInfo);
}  // namespace routing::turns::lanes
