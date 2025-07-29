#include "lane_way.hpp"

#include "base/assert.hpp"

namespace routing::turns::lanes
{
std::string DebugPrint(LaneWay const laneWay)
{
  using enum LaneWay;
  switch (laneWay)
  {
  case None: return "None";
  case Left: return "Left";
  case SlightLeft: return "SlightLeft";
  case SharpLeft: return "SharpLeft";
  case Through: return "Through";
  case Right: return "Right";
  case SlightRight: return "SlightRight";
  case SharpRight: return "SharpRight";
  case Reverse: return "Reverse";
  case MergeToLeft: return "MergeToLeft";
  case MergeToRight: return "MergeToRight";
  case Count: return "Count";
  default:
    ASSERT_FAIL("Unsupported value: " + std::to_string(static_cast<std::uint8_t>(laneWay)));
    return "Unsupported";
  }
}

std::string DebugPrint(LaneWays const & laneWays)
{
  std::stringstream out;
  out << "LaneWays: [";
  std::uint8_t const waysCount = laneWays.m_laneWays.count();
  std::uint8_t waysPrinted = 0;
  for (std::size_t i = 0; i < laneWays.m_laneWays.size(); ++i)
  {
    if (laneWays.m_laneWays.test(i))
    {
      out << DebugPrint(static_cast<LaneWay>(i));
      if (waysPrinted < waysCount - 1)
        out << ", ";
      waysPrinted++;
    }
  }
  out << "]";
  return out.str();
}
}  // namespace routing::turns::lanes
