#include "routing/lanes/lanes_collapse.hpp"

#include <algorithm>
#include <sstream>
#include <utility>

namespace routing::turns::lanes
{
namespace
{
bool IsNotRecommended(LaneInfo const & lane)
{
  return lane.recommendedWay == LaneWay::None;
}
}  // namespace

CollapsedLanes CollapseLanes(LanesInfo lanes)
{
  CollapsedLanes result;
  auto & out = result.lanes;

  // Junctions that already fit are rendered verbatim, one entry per physical lane.
  if (lanes.size() <= kMaxLanesToDisplay)
  {
    out = std::move(lanes);
    return result;
  }

  out.reserve(lanes.size());
  for (auto & lane : lanes)
    if (!out.empty() && IsNotRecommended(out.back()) && IsNotRecommended(lane) && out.back().laneWays == lane.laneWays)
      ++out.back().similarLanesCount;
    else
      out.push_back(std::move(lane));

  while (out.size() > kMaxLanesToDisplay)
  {
    if (IsNotRecommended(out.front()))
    {
      out.erase(out.begin());
      result.trimmedLeft = true;
    }
    else if (IsNotRecommended(out.back()))
    {
      out.pop_back();
      result.trimmedRight = true;
    }
    else
    {
      // Both edges are recommended: sacrifice an interior non-recommended entry first and a
      // recommended one only when nothing else is left. No edge flag is set for an interior
      // drop — the strip edges still show real outermost lanes.
      auto const it = std::find_if(out.begin() + 1, out.end() - 1, IsNotRecommended);
      if (it != out.end() - 1)
      {
        out.erase(it);
      }
      else
      {
        out.pop_back();
        result.trimmedRight = true;
      }
    }
  }

  return result;
}

std::string DebugPrint(CollapsedLanes const & collapsedLanes)
{
  std::stringstream out;
  out << "CollapsedLanes{" << DebugPrint(collapsedLanes.lanes) << ", trimmedLeft: " << collapsedLanes.trimmedLeft
      << ", trimmedRight: " << collapsedLanes.trimmedRight << "}";
  return out.str();
}
}  // namespace routing::turns::lanes
