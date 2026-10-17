#include "map/nogps/turn_matcher.hpp"

#include "map/nogps/geo.hpp"
#include "map/nogps/road_walker.hpp"

#include "base/math.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>

namespace nogps
{
namespace
{
// Returns true if a road leaves the crossing in the direction.
bool HasRoadFromCrossing(Roads & roads, ms::LatLon const & crossing, double bearingDeg)
{
  auto const point = Move(crossing, bearingDeg, TurnMatcher::kRoadCheckDistanceM);
  auto const road = roads.Snap(point, bearingDeg, TurnMatcher::kRoadCheckRadiusM);
  if (!road || Distance(point, road->m_point) > TurnMatcher::kRoadCheckRadiusM)
    return false;
  return std::fabs(AngleDiff(Orient(road->m_bearingDeg, bearingDeg), bearingDeg)) <= TurnMatcher::kMaxRoadDiffDeg;
}
}  // namespace

std::optional<ms::LatLon> TurnMatcher::FindCrossing(Roads & roads, ms::LatLon const & corner, double fromBearingDeg,
                                                    double toBearingDeg, double searchM, std::string * whyNot,
                                                    bool onlyOne, std::vector<ms::LatLon> * fittingOut)
{
  if (fittingOut)
    fittingOut->clear();
  auto const no = [whyNot](std::string const & why)
  {
    if (whyNot)
      *whyNot = why;
    return std::optional<ms::LatLon>();
  };
  double const turn = std::fabs(AngleDiff(toBearingDeg, fromBearingDeg));
  if (turn < kMinTurnDeg)
    return no("a turn that small is a bend of the road");
  if (turn > kMaxTurnDeg)
    return no("a turn that large is a turn around");

  double const radius = std::max(kMinSearchM, std::min(kMaxSearchM, searchM));
  // A bend of the road with a side street close to it would move the car to the street.
  if (IsBend(roads, corner, fromBearingDeg, toBearingDeg, radius))
    return no("the road bends that way");

  std::optional<ms::LatLon> best;
  double bestAlong = std::numeric_limits<double>::max();
  // The crossings that fit with how far along the road they are.
  std::vector<std::pair<double, ms::LatLon>> fitting;
  bool const all = onlyOne || fittingOut;
  auto const crossings = roads.FindCrossings(corner, radius);
  size_t aside = 0;
  size_t noRoadBack = 0;
  size_t noRoadAhead = 0;
  for (auto const & crossing : crossings)
  {
    // The offset of the crossing along the road the car came by and to the side of it.
    double const distance = Distance(corner, crossing);
    double const angle = math::DegToRad(AngleDiff(Bearing(corner, crossing), fromBearingDeg));
    double const along = std::fabs(distance * std::cos(angle));
    double const side = std::fabs(distance * std::sin(angle));
    if (along > radius || side > kMaxSideOffsetM)
    {
      ++aside;
      continue;
    }
    if (along >= bestAlong && !all)
      continue;
    if (!HasRoadFromCrossing(roads, crossing, fromBearingDeg + 180))
    {
      ++noRoadBack;
      continue;
    }
    if (!HasRoadFromCrossing(roads, crossing, toBearingDeg))
    {
      ++noRoadAhead;
      continue;
    }
    fitting.emplace_back(along, crossing);
    if (along >= bestAlong)
      continue;
    best = crossing;
    bestAlong = along;
  }
  if (fittingOut)
  {
    std::sort(fitting.begin(), fitting.end(), [](auto const & a, auto const & b) { return a.first < b.first; });
    for (auto const & item : fitting)
      fittingOut->push_back(item.second);
  }
  if (best && onlyOne)
  {
    for (auto const & item : fitting)
      if (Distance(item.second, *best) > kSameCrossingM)
        return no(std::to_string(fitting.size()) + " crossings fit, and the car was not on a road to tell which");
  }
  if (!best)
  {
    return no("crossings around " + std::to_string(crossings.size()) + ", farther than " +
              std::to_string(std::lround(radius)) + " m along the road or aside of it " + std::to_string(aside) +
              ", without the road the car came by " + std::to_string(noRoadBack) +
              ", without a road going the new way " + std::to_string(noRoadAhead));
  }
  return best;
}

std::string TurnMatcher::DescribeCrossings(Roads & roads, ms::LatLon const & corner, double fromBearingDeg,
                                           double toBearingDeg)
{
  struct Item
  {
    double m_distance;
    ms::LatLon m_point;
  };
  std::vector<Item> items;
  for (auto const & crossing : roads.FindCrossings(corner, kLogRadiusM))
    if (double const distance = Distance(corner, crossing); distance <= kLogRadiusM)
      items.push_back({distance, crossing});
  std::sort(items.begin(), items.end(), [](Item const & a, Item const & b) { return a.m_distance < b.m_distance; });
  if (items.size() > kLogCrossings)
    items.resize(kLogCrossings);

  std::string result;
  for (auto const & item : items)
  {
    double const angle = math::DegToRad(AngleDiff(Bearing(corner, item.m_point), fromBearingDeg));
    char buf[48];
    std::snprintf(buf, sizeof(buf), "%s%+ld/%+ld/%c%c", result.empty() ? "" : ",",
                  std::lround(item.m_distance * std::cos(angle)), std::lround(item.m_distance * std::sin(angle)),
                  HasRoadFromCrossing(roads, item.m_point, fromBearingDeg + 180) ? 'B' : '-',
                  HasRoadFromCrossing(roads, item.m_point, toBearingDeg) ? 'A' : '-');
    result += buf;
  }
  return result.empty() ? "-" : result;
}

bool TurnMatcher::IsBend(Roads & roads, ms::LatLon const & position, double fromBearingDeg, double toBearingDeg,
                         double distanceM)
{
  auto const start = roads.Snap(position, fromBearingDeg, RoadWalker::kSnapRadiusM);
  if (!start)
    return false;
  ms::LatLon pos = start->m_point;
  double heading = Orient(start->m_bearingDeg, fromBearingDeg);
  for (double walked = 0; walked < distanceM; walked += RoadWalker::kStepM)
  {
    if (std::fabs(AngleDiff(toBearingDeg, heading)) <= kMaxBendEndDiffDeg)
      return true;
    auto const road = roads.Snap(Move(pos, heading, RoadWalker::kStepM), heading, RoadWalker::kSnapRadiusM);
    if (!road)
      return false;
    double const roadHeading = Orient(road->m_bearingDeg, heading);
    if (std::fabs(AngleDiff(roadHeading, heading)) > RoadWalker::kMaxBendDeg)
      return false;
    pos = road->m_point;
    heading = roadHeading;
  }
  return std::fabs(AngleDiff(toBearingDeg, heading)) <= kMaxBendEndDiffDeg;
}
}  // namespace nogps
