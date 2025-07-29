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
  case ReverseLeft: return "ReverseLeft";
  case SharpLeft: return "SharpLeft";
  case Left: return "Left";
  case MergeToLeft: return "MergeToLeft";
  case SlightLeft: return "SlightLeft";
  case Through: return "Through";
  case SlightRight: return "SlightRight";
  case MergeToRight: return "MergeToRight";
  case Right: return "Right";
  case SharpRight: return "SharpRight";
  case ReverseRight: return "ReverseRight";
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
