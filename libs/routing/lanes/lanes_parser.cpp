#include "lanes_parser.hpp"

#include <algorithm>
#include <ranges>

namespace routing::turns::lanes
{
namespace
{
std::uint8_t constexpr kLaneWayNamesCount = static_cast<std::uint8_t>(LaneWay::Count) + 4;

/**
 * The order is important. Starting with the most frequent tokens according to
 * taginfo.openstreetmap.org we minimize the number of the comparisons in ParseSingleLane().
 *
 * A `none` lane can be represented either as "none" or as "". That means both "none" and ""
 * should be considered names, even though they refer to the same thing. As a result,
 * `LaneWay::None` appears twice in this array, which is one longer than the number of
 * enum values.
 */
std::array<std::pair<LaneWay, std::string_view>, kLaneWayNamesCount> constexpr g_laneWayNames{{
    {LaneWay::None, ""},
    {LaneWay::Through, "through"},
    {LaneWay::Left, "left"},
    {LaneWay::Right, "right"},
    {LaneWay::None, "none"},
    {LaneWay::SharpLeft, "sharp_left"},
    {LaneWay::SlightLeft, "slight_left"},
    {LaneWay::MergeToRight, "merge_to_right"},
    {LaneWay::MergeToLeft, "merge_to_left"},
    {LaneWay::SlightRight, "slight_right"},
    {LaneWay::SharpRight, "sharp_right"},
    {LaneWay::ReverseLeft, "reverse"},
    {LaneWay::Right,
     "next_right"},  // "next_right" means "turn right, not in the first intersection but the one after that".
    {LaneWay::Through, "slide_left"},  // "slide_left" means "move a bit left within the lane".
    {LaneWay::Through, "slide_right"}  // "slide_right" means "move a bit right within the lane".
}};

bool ParseSingleLane(auto && laneWayRange, LaneWay & laneWay)
{
  auto const it = std::ranges::find_if(
      g_laneWayNames, [&laneWayRange](auto const & pair) { return std::ranges::equal(laneWayRange, pair.second); });
  if (it != g_laneWayNames.end())
  {
    laneWay = it->first;
    return true;
  }
  return false;
}

}  // namespace

LanesInfo ParseLanes(std::string_view lanesString)
{
  if (lanesString.empty())
    return {};

  LanesInfo lanes;
  for (auto && laneInfo : lanesString | std::views::split('|'))
  {
    LaneInfo lane;
    if (std::ranges::empty(laneInfo))
      lane.laneWays.Add(LaneWay::None);
    else
    {
      for (auto && laneWay : laneInfo | std::views::split(';'))
      {
        auto way = LaneWay::None;
        auto && laneWayProcessed = laneWay | std::views::filter([](char const c) { return !std::isspace(c); }) |
                                   std::views::transform([](char const c) { return std::tolower(c); });
        if (!ParseSingleLane(laneWayProcessed, way))
          return {};
        lane.laneWays.Add(way);
        if (way == LaneWay::ReverseLeft)
          lane.laneWays.Add(LaneWay::ReverseRight);
      }
    }

    lanes.push_back(lane);
  }
  return lanes;
}
}  // namespace routing::turns::lanes
