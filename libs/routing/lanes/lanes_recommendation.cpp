#include "lanes_recommendation.hpp"

#include "routing/route.hpp"

namespace routing::turns::lanes
{
void SelectRecommendedLanes(std::vector<RouteSegment> & routeSegments)
{
  for (auto & segment : routeSegments)
  {
    auto & t = segment.GetTurn();
    if (t.IsTurnNone() || t.m_lanes.empty())
      continue;
    auto & lanesInfo = segment.GetTurnLanes();
    // Check if there are elements in lanesInfo that correspond with the turn exactly.
    // If so, fix up all the elements in lanesInfo that correspond with the turn.
    if (impl::SetRecommendedLaneWays(t.m_turn, lanesInfo))
      continue;
    // If not, check if there are elements in lanesInfo that correspond with the turn
    // approximately. If so, fix up all those elements.
    if (impl::SetRecommendedLaneWaysApproximately(t.m_turn, lanesInfo))
      continue;
    // If not, check if there is an unrestricted lane that could correspond to the
    // turn. If so, fix up that lane.
    if (impl::SetUnrestrictedLaneAsRecommended(t.m_turn, lanesInfo))
      continue;
    // Otherwise, we don't have lane recommendations for the user, so we don't
    // want to send the lane data any further.
    segment.ClearTurnLanes();
  }
}

bool impl::SetRecommendedLaneWays(CarDirection const carDirection, LanesInfo & lanesInfo)
{
  LaneWay laneWay;
  switch (carDirection)
  {
  case CarDirection::GoStraight: laneWay = LaneWay::Through; break;
  case CarDirection::TurnRight: laneWay = LaneWay::Right; break;
  case CarDirection::TurnSharpRight: laneWay = LaneWay::SharpRight; break;
  case CarDirection::TurnSlightRight: [[fallthrough]];
  case CarDirection::ExitHighwayToRight: laneWay = LaneWay::SlightRight; break;
  case CarDirection::TurnLeft: laneWay = LaneWay::Left; break;
  case CarDirection::TurnSharpLeft: laneWay = LaneWay::SharpLeft; break;
  case CarDirection::TurnSlightLeft: [[fallthrough]];
  case CarDirection::ExitHighwayToLeft: laneWay = LaneWay::SlightLeft; break;
  case CarDirection::UTurnLeft: [[fallthrough]];
  case CarDirection::UTurnRight: laneWay = LaneWay::Reverse; break;
  default: return false;
  }

  bool isLaneConformed = false;
  for (auto & [laneWays, recommendedWay] : lanesInfo)
  {
    if (laneWays.Contains(laneWay))
    {
      recommendedWay = laneWay;
      isLaneConformed = true;
    }
  }
  return isLaneConformed;
}

bool impl::SetRecommendedLaneWaysApproximately(CarDirection const carDirection, LanesInfo & lanesInfo)
{
  std::vector<LaneWay> approximateLaneWays;
  switch (carDirection)
  {
  case CarDirection::GoStraight:
    approximateLaneWays = {LaneWay::Through, LaneWay::SlightRight, LaneWay::SlightLeft};
    break;
  case CarDirection::TurnRight:
    approximateLaneWays = {LaneWay::Right, LaneWay::SharpRight, LaneWay::SlightRight};
    break;
  case CarDirection::TurnSharpRight: approximateLaneWays = {LaneWay::SharpRight, LaneWay::Right}; break;
  case CarDirection::TurnSlightRight:
    approximateLaneWays = {LaneWay::SlightRight, LaneWay::Through, LaneWay::Right};
    break;
  case CarDirection::TurnLeft: approximateLaneWays = {LaneWay::Left, LaneWay::SlightLeft, LaneWay::SharpLeft}; break;
  case CarDirection::TurnSharpLeft: approximateLaneWays = {LaneWay::SharpLeft, LaneWay::Left}; break;
  case CarDirection::TurnSlightLeft:
    approximateLaneWays = {LaneWay::SlightLeft, LaneWay::Through, LaneWay::Left};
    break;
  case CarDirection::UTurnLeft: [[fallthrough]];
  case CarDirection::UTurnRight: approximateLaneWays = {LaneWay::Reverse}; break;
  case CarDirection::ExitHighwayToLeft: approximateLaneWays = {LaneWay::SlightLeft, LaneWay::Left}; break;
  case CarDirection::ExitHighwayToRight: approximateLaneWays = {LaneWay::SlightRight, LaneWay::Right}; break;
  default: return false;
  }

  bool isLaneConformed = false;
  for (auto & [laneWays, recommendedWay] : lanesInfo)
  {
    for (auto const & laneWay : approximateLaneWays)
    {
      if (laneWays.Contains(laneWay))
      {
        recommendedWay = laneWay;
        isLaneConformed = true;
        break;
      }
    }
  }

  return isLaneConformed;
}

bool impl::SetUnrestrictedLaneAsRecommended(CarDirection carDirection, LanesInfo & lanesInfo)
{
  return false;
}
}  // namespace routing::turns::lanes
