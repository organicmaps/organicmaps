#include "lane_info.hpp"

#include <sstream>

namespace routing::turns::lanes
{
std::string DebugPrint(LaneInfo const & laneInfo)
{
  std::stringstream out;
  out << "LaneInfo{" << DebugPrint(laneInfo.laneWays) << ", recommendedWay: " << DebugPrint(laneInfo.recommendedWay)
      << "}";
  return out.str();
}

std::string DebugPrint(LanesInfo const & lanesInfo)
{
  std::stringstream out;
  out << "LanesInfo[";
  for (auto const & laneInfo : lanesInfo)
    out << DebugPrint(laneInfo) << ", ";
  out << "]";
  return out.str();
}
}  // namespace routing::turns::lanes
