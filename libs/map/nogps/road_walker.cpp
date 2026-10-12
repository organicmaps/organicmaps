#include "map/nogps/road_walker.hpp"

#include "map/nogps/geo.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace nogps
{
namespace
{
std::optional<ms::LatLon> FindCrossingOnPiece(std::vector<ms::LatLon> const & crossings, ms::LatLon const & start,
                                              ms::LatLon const & from, ms::LatLon const & to)
{
  std::optional<ms::LatLon> closest;
  double closestDistance = std::numeric_limits<double>::max();
  for (auto const & crossing : crossings)
  {
    if (Distance(crossing, start) < RoadWalker::kSameCrossingM)
      continue;
    if (DistanceToPiece(crossing, from, to) > RoadWalker::kCrossingOnRoadM)
      continue;
    double const d = Distance(from, crossing);
    if (d < closestDistance)
    {
      closestDistance = d;
      closest = crossing;
    }
  }
  return closest;
}
}  // namespace

std::optional<RoadWalker::Result> RoadWalker::Walk(Roads & roads, ms::LatLon const & position, double bearingDeg,
                                                   double distanceM)
{
  bool const forward = distanceM >= 0;
  double heading = forward ? bearingDeg : bearingDeg + 180;
  auto const start = roads.Snap(position, heading, kSnapRadiusM);
  if (!start)
    return {};

  ms::LatLon pos = start->m_point;
  heading = Orient(start->m_bearingDeg, heading);
  auto const crossings = roads.FindCrossings(pos, std::fabs(distanceM) + kCrossingSearchMarginM);

  double remaining = std::fabs(distanceM);
  double applied = 0;
  bool atCrossing = false;
  while (remaining > 0)
  {
    double const step = std::min(kStepM, remaining);
    auto const road = roads.Snap(Move(pos, heading, step), heading, kSnapRadiusM);
    if (!road)
      break;
    double const roadHeading = Orient(road->m_bearingDeg, heading);
    if (std::fabs(AngleDiff(roadHeading, heading)) > kMaxBendDeg)
      break;

    if (auto const crossing = FindCrossingOnPiece(crossings, start->m_point, pos, road->m_point))
    {
      applied += Distance(pos, *crossing);
      pos = *crossing;
      atCrossing = true;
      break;
    }

    applied += Distance(pos, road->m_point);
    pos = road->m_point;
    heading = roadHeading;
    remaining -= step;
  }

  return Result{pos, Normalize(forward ? heading : heading + 180), forward ? applied : -applied, atCrossing};
}
}  // namespace nogps
