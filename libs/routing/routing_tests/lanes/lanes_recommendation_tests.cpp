#include "routing/turns.hpp"
#include "testing/testing.hpp"

#include "routing/lanes/lanes_recommendation.hpp"

namespace routing::turns::lanes::test
{
UNIT_TEST(TestSetRecommendedLaneWays_Smoke)
{
  using impl::SetRecommendedLaneWays;

  struct CarDirectionToLaneWayMapping
  {
    CarDirection carDirection;
    LaneWay laneWay;
    bool shouldBeRecommended;
  };
  std::vector<CarDirectionToLaneWayMapping> const testData = {
      {CarDirection::GoStraight, LaneWay::Through, true},
      {CarDirection::TurnRight, LaneWay::Right, true},
      {CarDirection::TurnSharpRight, LaneWay::SharpRight, true},
      {CarDirection::TurnSlightRight, LaneWay::SlightRight, true},
      {CarDirection::TurnLeft, LaneWay::Left, true},
      {CarDirection::TurnSharpLeft, LaneWay::SharpLeft, true},
      {CarDirection::TurnSlightLeft, LaneWay::SlightLeft, true},
      {CarDirection::UTurnLeft, LaneWay::Reverse, true},
      {CarDirection::UTurnRight, LaneWay::Reverse, true},
      {CarDirection::ExitHighwayToLeft, LaneWay::SlightLeft, true},
      {CarDirection::ExitHighwayToRight, LaneWay::SlightRight, true},
      // We do not recommend any lane way for these directions
      {CarDirection::None, LaneWay::None, false},
      {CarDirection::EnterRoundAbout, LaneWay::None, false},
      {CarDirection::LeaveRoundAbout, LaneWay::None, false},
      {CarDirection::StayOnRoundAbout, LaneWay::None, false},
      {CarDirection::StartAtEndOfStreet, LaneWay::None, false},
      {CarDirection::ReachedYourDestination, LaneWay::None, false},
  };
  TEST_EQUAL(testData.size(), static_cast<size_t>(CarDirection::Count), ("Not all CarDirection values are covered."));
  for (auto const & [carDirection, laneWay, shouldBeRecommended] : testData)
  {
    LanesInfo lanesInfo = {{{laneWay}}};
    bool const isRecommended = SetRecommendedLaneWays(carDirection, lanesInfo);
    TEST_EQUAL(isRecommended, shouldBeRecommended,
               ("CarDirection:", DebugPrint(carDirection), "LaneWay:", DebugPrint(laneWay)));
    TEST_EQUAL(lanesInfo[0].recommendedWay, shouldBeRecommended ? laneWay : LaneWay::None, ());
  }
}

UNIT_TEST(TestSetRecommendedLaneWays)
{
  //   TEST(IsLaneWayConformedTurnDirection(LaneWay::Left, CarDirection::TurnLeft), ());
  //   TEST(IsLaneWayConformedTurnDirection(LaneWay::Right, CarDirection::TurnRight), ());
  //   TEST(IsLaneWayConformedTurnDirection(LaneWay::SlightLeft, CarDirection::TurnSlightLeft), ());
  //   TEST(IsLaneWayConformedTurnDirection(LaneWay::SharpRight, CarDirection::TurnSharpRight), ());
  //   TEST(IsLaneWayConformedTurnDirection(LaneWay::Reverse, CarDirection::UTurnLeft), ());
  //   TEST(IsLaneWayConformedTurnDirection(LaneWay::Reverse, CarDirection::UTurnRight), ());
  //   TEST(IsLaneWayConformedTurnDirection(LaneWay::Through, CarDirection::GoStraight), ());
  //   TEST(!IsLaneWayConformedTurnDirection(LaneWay::Left, CarDirection::TurnSlightLeft), ());
  //   TEST(!IsLaneWayConformedTurnDirection(LaneWay::Right, CarDirection::TurnSharpRight), ());
  //   TEST(!IsLaneWayConformedTurnDirection(LaneWay::SlightLeft, CarDirection::GoStraight), ());
  //   TEST(!IsLaneWayConformedTurnDirection(LaneWay::SharpRight, CarDirection::None), ());
  //   TEST(!IsLaneWayConformedTurnDirection(LaneWay::Reverse, CarDirection::TurnLeft), ());
  //   TEST(!IsLaneWayConformedTurnDirection(LaneWay::None, CarDirection::ReachedYourDestination), ());
}

UNIT_TEST(SetRecommendedLaneWaysApproximately_Smoke) {}

UNIT_TEST(SetRecommendedLaneWaysApproximately)
{
  //   TEST(IsLaneWayConformedTurnDirectionApproximately(LaneWay::Left, CarDirection::TurnSharpLeft), ());
  //   TEST(IsLaneWayConformedTurnDirectionApproximately(LaneWay::Left, CarDirection::TurnSlightLeft), ());
  //   TEST(IsLaneWayConformedTurnDirectionApproximately(LaneWay::Right, CarDirection::TurnSharpRight), ());
  //   TEST(IsLaneWayConformedTurnDirectionApproximately(LaneWay::Right, CarDirection::TurnRight), ());
  //   TEST(IsLaneWayConformedTurnDirectionApproximately(LaneWay::Reverse, CarDirection::UTurnLeft), ());
  //   TEST(IsLaneWayConformedTurnDirectionApproximately(LaneWay::Reverse, CarDirection::UTurnRight), ());
  //   TEST(IsLaneWayConformedTurnDirectionApproximately(LaneWay::SlightLeft, CarDirection::GoStraight), ());
  //   TEST(IsLaneWayConformedTurnDirectionApproximately(LaneWay::SlightRight, CarDirection::GoStraight), ());
  //   TEST(!IsLaneWayConformedTurnDirectionApproximately(LaneWay::SharpLeft, CarDirection::UTurnLeft), ());
  //   TEST(!IsLaneWayConformedTurnDirectionApproximately(LaneWay::SharpLeft, CarDirection::UTurnRight), ());
  //   TEST(!IsLaneWayConformedTurnDirectionApproximately(LaneWay::SharpRight, CarDirection::UTurnLeft), ());
  //   TEST(!IsLaneWayConformedTurnDirectionApproximately(LaneWay::SharpRight, CarDirection::UTurnRight), ());
  //   TEST(!IsLaneWayConformedTurnDirection(LaneWay::Through, CarDirection::ReachedYourDestination), ());
  //   TEST(!IsLaneWayConformedTurnDirectionApproximately(LaneWay::Through, CarDirection::TurnRight), ());
  //   TEST(!IsLaneWayConformedTurnDirectionApproximately(LaneWay::SlightRight, CarDirection::TurnSharpLeft), ());
}

UNIT_TEST(SetUnrestrictedLaneAsRecommended_Smoke) {}

UNIT_TEST(SetUnrestrictedLaneAsRecommended) {}

UNIT_TEST(SelectRecommendedLanes)
{
  //   vector<TurnItem> turns = {{1, CarDirection::GoStraight},
  //                             {2, CarDirection::TurnLeft},
  //                             {3, CarDirection::TurnRight},
  //                             {4, CarDirection::ReachedYourDestination}};
  //   turns[0].m_lanes.push_back({LaneWay::Left, LaneWay::Through});
  //   turns[0].m_lanes.push_back({LaneWay::Right});
  //   turns[1].m_lanes.push_back({LaneWay::SlightLeft});
  //   turns[1].m_lanes.push_back({LaneWay::Through});
  //   turns[1].m_lanes.push_back({LaneWay::None});
  //   turns[2].m_lanes.push_back({LaneWay::Left, LaneWay::SharpLeft});
  //   turns[2].m_lanes.push_back({LaneWay::None});
  //   vector<RouteSegment> routeSegments;
  //   RouteSegmentsFrom({}, {}, turns, {}, routeSegments);
  //   SelectRecommendedLanes(routeSegments);
  //   TEST(routeSegments[0].GetTurn().m_lanes[0].m_isRecommended, ());
  //   TEST(!routeSegments[0].GetTurn().m_lanes[1].m_isRecommended, ());
  //   TEST(routeSegments[1].GetTurn().m_lanes[0].m_isRecommended, ());
  //   TEST(!routeSegments[1].GetTurn().m_lanes[1].m_isRecommended, ());
  //   TEST(!routeSegments[1].GetTurn().m_lanes[2].m_isRecommended, ());
  //   TEST(!routeSegments[2].GetTurn().m_lanes[0].m_isRecommended, ());
  //   TEST(routeSegments[2].GetTurn().m_lanes[1].m_isRecommended, ());
}
}  // namespace routing::turns::lanes::test
