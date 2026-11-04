#include "lanes_recommendation.hpp"

#include "routing/route.hpp"

namespace routing::turns::lanes
{
namespace
{
void FixRecommendedReverseLane(LaneWays & ways, LaneWay const recommendedWay)
{
  if (recommendedWay == LaneWay::ReverseLeft)
    ways.Remove(LaneWay::ReverseRight);
  else if (recommendedWay == LaneWay::ReverseRight)
    ways.Remove(LaneWay::ReverseLeft);
}

void RecommendLanes(CarDirection direction, LanesInfo & lanes)
{
  if (!impl::SetRecommendedLaneWays(direction, lanes) && !impl::SetRecommendedLaneWaysApproximately(direction, lanes) &&
      !impl::SetUnrestrictedLaneAsRecommended(direction, lanes))
    lanes.clear();
}

void PropagateRecommendedLanes(LanesInfo const & next, size_t offset, LanesInfo & previous)
{
  if (next.empty())
  {
    previous.clear();
    return;
  }

  ASSERT_LESS_OR_EQUAL(offset + next.size(), previous.size(), ());
  for (size_t i = 0; i < next.size(); ++i)
  {
    auto const way = next[i].recommendedWay;
    if (way == LaneWay::None)
      continue;

    auto & lane = previous[offset + i];
    lane.recommendedWay = way;
    FixRecommendedReverseLane(lane.laneWays, way);
  }
}
}  // namespace

void SelectRecommendedLanes(std::vector<RouteSegment> & routeSegments)
{
  for (auto & segment : routeSegments)
  {
    auto & t = segment.GetTurn();
    if (t.IsTurnNone())
      continue;

    auto & lanesInfo = segment.GetTurnLanes();
    RecommendLanes(t.m_turn, lanesInfo);
    auto const * next = &lanesInfo;
    for (auto & layout : segment.GetApproachLanes())
    {
      PropagateRecommendedLanes(*next, layout.m_offset, layout.m_lanes);
      next = &layout.m_lanes;
    }
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
  case CarDirection::UTurnLeft: laneWay = LaneWay::ReverseLeft; break;
  case CarDirection::UTurnRight: laneWay = LaneWay::ReverseRight; break;
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
    FixRecommendedReverseLane(laneWays, recommendedWay);
  }
  return isLaneConformed;
}

bool impl::SetRecommendedLaneWaysApproximately(CarDirection const carDirection, LanesInfo & lanesInfo)
{
  std::vector<LaneWay> approximateLaneWays;
  switch (carDirection)
  {
  case CarDirection::UTurnLeft: approximateLaneWays = {LaneWay::SharpLeft}; break;
  case CarDirection::TurnSharpLeft: approximateLaneWays = {LaneWay::Left}; break;
  case CarDirection::TurnLeft: approximateLaneWays = {LaneWay::SlightLeft, LaneWay::SharpLeft}; break;
  case CarDirection::TurnSlightLeft: [[fallthrough]];
  case CarDirection::ExitHighwayToLeft: approximateLaneWays = {LaneWay::Left}; break;
  case CarDirection::GoStraight: approximateLaneWays = {LaneWay::SlightRight, LaneWay::SlightLeft}; break;
  case CarDirection::ExitHighwayToRight: [[fallthrough]];
  case CarDirection::TurnSlightRight: approximateLaneWays = {LaneWay::Right}; break;
  case CarDirection::TurnRight: approximateLaneWays = {LaneWay::SlightRight, LaneWay::SharpRight}; break;
  case CarDirection::TurnSharpRight: approximateLaneWays = {LaneWay::Right}; break;
  case CarDirection::UTurnRight: approximateLaneWays = {LaneWay::SharpRight}; break;
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

bool impl::SetUnrestrictedLaneAsRecommended(CarDirection const carDirection, LanesInfo & lanesInfo)
{
  static auto constexpr setFirstUnrestrictedLane = [](LaneWay const laneWay, auto beginIt, auto endIt)
  {
    auto it = std::find_if(beginIt, endIt, [](auto const & laneInfo) { return laneInfo.laneWays.IsUnrestricted(); });
    if (it == endIt)
      return false;
    it->recommendedWay = laneWay;
    return true;
  };

  if (IsTurnMadeFromLeft(carDirection))
    return setFirstUnrestrictedLane(LaneWay::Left, lanesInfo.begin(), lanesInfo.end());
  if (IsTurnMadeFromRight(carDirection))
    return setFirstUnrestrictedLane(LaneWay::Right, lanesInfo.rbegin(), lanesInfo.rend());
  return false;
}
}  // namespace routing::turns::lanes
