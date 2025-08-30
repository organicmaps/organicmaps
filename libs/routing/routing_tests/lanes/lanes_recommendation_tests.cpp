#include "routing/turns.hpp"
#include "testing/testing.hpp"

#include "routing/lanes/lanes_recommendation.hpp"
#include "routing/routing_tests/tools.hpp"

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
      {CarDirection::UTurnLeft, LaneWay::ReverseLeft, true},
      {CarDirection::UTurnRight, LaneWay::ReverseRight, true},
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
  {
    LanesInfo lanesInfo = {
        {{LaneWay::ReverseLeft, LaneWay::Left, LaneWay::Through}},
        {{LaneWay::Through}},
        {{LaneWay::Through}},
        {{LaneWay::Through, LaneWay::Right}},
        {{LaneWay::Right}},
    };
    TEST(impl::SetRecommendedLaneWays(CarDirection::GoStraight, lanesInfo), ());
    TEST_EQUAL(lanesInfo[0].recommendedWay, LaneWay::Through, ());
    TEST_EQUAL(lanesInfo[1].recommendedWay, LaneWay::Through, ());
    TEST_EQUAL(lanesInfo[2].recommendedWay, LaneWay::Through, ());
    TEST_EQUAL(lanesInfo[3].recommendedWay, LaneWay::Through, ());
    TEST_EQUAL(lanesInfo[4].recommendedWay, LaneWay::None, ());
  }
  {
    LanesInfo lanesInfo = {
        {{LaneWay::ReverseLeft, LaneWay::Left}},
        {{LaneWay::Right}},
    };
    TEST(!impl::SetRecommendedLaneWays(CarDirection::GoStraight, lanesInfo), ());
    TEST_EQUAL(lanesInfo[0].recommendedWay, LaneWay::None, ());
    TEST_EQUAL(lanesInfo[1].recommendedWay, LaneWay::None, ());
  }
  {
    LanesInfo lanesInfo = {
        {{LaneWay::ReverseLeft, LaneWay::ReverseRight}},
    };
    TEST(impl::SetRecommendedLaneWays(CarDirection::UTurnLeft, lanesInfo), ());
    TEST_EQUAL(lanesInfo[0].recommendedWay, LaneWay::ReverseLeft, ());
    TEST_EQUAL(lanesInfo[0].laneWays.Contains(LaneWay::ReverseRight), false, ());
  }
}

UNIT_TEST(SetRecommendedLaneWaysApproximately_Smoke)
{
  {
    struct CarDirectionToLaneWaysApproximateMapping
    {
      CarDirection carDirection;
      std::vector<LaneWay> laneWay;
    };
    std::vector<CarDirectionToLaneWaysApproximateMapping> const testData = {
        {CarDirection::UTurnLeft, {LaneWay::SharpLeft}},
        {CarDirection::TurnSharpLeft, {LaneWay::Left}},
        {CarDirection::TurnLeft, {LaneWay::SlightLeft, LaneWay::SharpLeft}},
        {CarDirection::TurnSlightLeft, {LaneWay::Left}},
        {CarDirection::ExitHighwayToLeft, {LaneWay::Left}},
        {CarDirection::GoStraight, {LaneWay::SlightRight, LaneWay::SlightLeft}},
        {CarDirection::ExitHighwayToRight, {LaneWay::Right}},
        {CarDirection::TurnSlightRight, {LaneWay::Right}},
        {CarDirection::TurnRight, {LaneWay::SlightRight, LaneWay::SharpRight}},
        {CarDirection::TurnSharpRight, {LaneWay::Right}},
        {CarDirection::UTurnRight, {LaneWay::SharpRight}},
    };
    for (auto const & [carDirection, laneWays] : testData)
    {
      for (auto const & laneWay : laneWays)
      {
        LanesInfo lanesInfo = {{{laneWay}}};
        bool const isRecommended = impl::SetRecommendedLaneWaysApproximately(carDirection, lanesInfo);
        TEST(isRecommended, ("CarDirection:", DebugPrint(carDirection), "LaneWay:", DebugPrint(laneWay)));
        TEST_EQUAL(lanesInfo[0].recommendedWay, laneWay, ());
      }
    }
  }

  {
    // Those directions do not have any recommended lane ways.
    std::vector const carDirections = {CarDirection::None,
                                       CarDirection::EnterRoundAbout,
                                       CarDirection::LeaveRoundAbout,
                                       CarDirection::StayOnRoundAbout,
                                       CarDirection::StartAtEndOfStreet,
                                       CarDirection::ReachedYourDestination};
    for (auto const & carDirection : carDirections)
    {
      LanesInfo lanesInfo = {{{LaneWay::Through}}};
      TEST(!impl::SetRecommendedLaneWaysApproximately(carDirection, lanesInfo), ());
      TEST_EQUAL(lanesInfo[0].recommendedWay, LaneWay::None, ());
    }
  }
}

UNIT_TEST(SetRecommendedLaneWaysApproximately)
{
  {
    LanesInfo lanesInfo = {
        {{LaneWay::ReverseLeft, LaneWay::Left, LaneWay::SlightLeft}},
        {{LaneWay::SlightRight, LaneWay::Right}},
        {{LaneWay::Right}},
    };
    TEST(impl::SetRecommendedLaneWaysApproximately(CarDirection::GoStraight, lanesInfo), ());
    TEST_EQUAL(lanesInfo[0].recommendedWay, LaneWay::SlightLeft, ());
    TEST_EQUAL(lanesInfo[1].recommendedWay, LaneWay::SlightRight, ());
    TEST_EQUAL(lanesInfo[2].recommendedWay, LaneWay::None, ());
  }
  {
    LanesInfo lanesInfo = {
        {{LaneWay::ReverseLeft, LaneWay::Left}},
        {{LaneWay::Right}},
    };
    TEST(!impl::SetRecommendedLaneWaysApproximately(CarDirection::GoStraight, lanesInfo), ());
    TEST_EQUAL(lanesInfo[0].recommendedWay, LaneWay::None, ());
    TEST_EQUAL(lanesInfo[1].recommendedWay, LaneWay::None, ());
  }
  {
    LanesInfo lanesInfo = {
        {{LaneWay::SharpLeft, LaneWay::SlightLeft}},
    };
    TEST(impl::SetRecommendedLaneWaysApproximately(CarDirection::TurnLeft, lanesInfo), ());
    TEST_EQUAL(lanesInfo[0].recommendedWay, LaneWay::SlightLeft, ());
  }
}

UNIT_TEST(SetUnrestrictedLaneAsRecommended)
{
  LanesInfo const testData = {{{LaneWay::ReverseLeft}}, {{LaneWay::None}}, {{LaneWay::None}}, {{LaneWay::Right}}};
  {
    LanesInfo lanesInfo = testData;
    TEST(impl::SetUnrestrictedLaneAsRecommended(CarDirection::TurnLeft, lanesInfo), ());
    TEST_EQUAL(lanesInfo[0].recommendedWay, LaneWay::None, ());
    TEST_EQUAL(lanesInfo[1].recommendedWay, LaneWay::Left, ());
    TEST_EQUAL(lanesInfo[2].recommendedWay, LaneWay::None, ());
    TEST_EQUAL(lanesInfo[3].recommendedWay, LaneWay::None, ());
  }
  {
    LanesInfo lanesInfo = testData;
    TEST(impl::SetUnrestrictedLaneAsRecommended(CarDirection::TurnRight, lanesInfo), ());
    TEST_EQUAL(lanesInfo[0].recommendedWay, LaneWay::None, ());
    TEST_EQUAL(lanesInfo[1].recommendedWay, LaneWay::None, ());
    TEST_EQUAL(lanesInfo[2].recommendedWay, LaneWay::Right, ());
    TEST_EQUAL(lanesInfo[3].recommendedWay, LaneWay::None, ());
  }
  {
    LanesInfo lanesInfo = {};
    TEST(!impl::SetUnrestrictedLaneAsRecommended(CarDirection::TurnRight, lanesInfo), ());
  }
  {
    LanesInfo lanesInfo = {{{LaneWay::Right}}};
    TEST(!impl::SetUnrestrictedLaneAsRecommended(CarDirection::TurnRight, lanesInfo), ());
    TEST_EQUAL(lanesInfo[0].recommendedWay, LaneWay::None, ());
  }
}

UNIT_TEST(SelectRecommendedLanes)
{
  std::vector<TurnItem> turns = {{1, CarDirection::GoStraight},
                                 {2, CarDirection::TurnLeft},
                                 {3, CarDirection::TurnRight},
                                 {4, CarDirection::ReachedYourDestination}};
  turns[0].m_lanes.push_back({{LaneWay::Left, LaneWay::Through}});
  turns[0].m_lanes.push_back({{LaneWay::Right}});
  turns[1].m_lanes.push_back({{LaneWay::SlightLeft}});
  turns[1].m_lanes.push_back({{LaneWay::Through}});
  turns[1].m_lanes.push_back({{LaneWay::None}});
  turns[2].m_lanes.push_back({{LaneWay::Left, LaneWay::SharpLeft}});
  turns[2].m_lanes.push_back({{LaneWay::None}});
  std::vector<RouteSegment> routeSegments;
  RouteSegmentsFrom({}, {}, turns, {}, routeSegments);
  SelectRecommendedLanes(routeSegments);
  TEST_EQUAL(routeSegments[0].GetTurn().m_lanes[0].recommendedWay, LaneWay::Through, ());
  TEST_EQUAL(routeSegments[0].GetTurn().m_lanes[1].recommendedWay, LaneWay::None, ());
  TEST_EQUAL(routeSegments[1].GetTurn().m_lanes[0].recommendedWay, LaneWay::SlightLeft, ());
  TEST_EQUAL(routeSegments[1].GetTurn().m_lanes[1].recommendedWay, LaneWay::None, ());
  TEST_EQUAL(routeSegments[1].GetTurn().m_lanes[2].recommendedWay, LaneWay::None, ());
  TEST_EQUAL(routeSegments[2].GetTurn().m_lanes[0].recommendedWay, LaneWay::None, ());
  TEST_EQUAL(routeSegments[2].GetTurn().m_lanes[1].recommendedWay, LaneWay::Right, ());
}
}  // namespace routing::turns::lanes::test
