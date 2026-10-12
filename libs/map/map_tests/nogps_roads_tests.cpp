#include "testing/testing.hpp"

#include "map/map_tests/nogps_test_roads.hpp"

#include "map/nogps/geo.hpp"
#include "map/nogps/road_walker.hpp"
#include "map/nogps/turn_matcher.hpp"

#include "base/math.hpp"

#include <cmath>
#include <string>

namespace nogps_roads_tests
{
using namespace nogps;
using namespace nogps_test;

void TestAt(double eastM, double northM, ms::LatLon const & position)
{
  TEST_ALMOST_EQUAL_ABS(East(position), eastM, 0.5, (position));
  TEST_ALMOST_EQUAL_ABS(North(position), northM, 0.5, (position));
}

UNIT_TEST(NoGps_RoadWalker_WalksAlongRoad)
{
  GridRoads grid;
  auto const walked = RoadWalker::Walk(grid, At(10, 0), 90, 50);
  TEST(walked, ());
  TestAt(60, 0, walked->m_position);
  TEST_ALMOST_EQUAL_ABS(walked->m_bearingDeg, 90.0, 1.0, ());
  TEST_ALMOST_EQUAL_ABS(walked->m_appliedM, 50.0, 0.5, ());
  TEST(!walked->m_atCrossing, ());
}

UNIT_TEST(NoGps_RoadWalker_SnapsToRoadAxis)
{
  GridRoads grid;
  auto const walked = RoadWalker::Walk(grid, At(10, 4), 90, 20);
  TEST(walked, ());
  TestAt(30, 0, walked->m_position);
}

UNIT_TEST(NoGps_RoadWalker_StopsAtCrossing)
{
  GridRoads grid;
  auto const walked = RoadWalker::Walk(grid, At(60, 0), 90, 100);
  TEST(walked, ());
  TestAt(100, 0, walked->m_position);
  TEST_ALMOST_EQUAL_ABS(walked->m_appliedM, 40.0, 0.5, ());
  TEST(walked->m_atCrossing, ());
}

UNIT_TEST(NoGps_RoadWalker_WalksBackKeepingDirection)
{
  GridRoads grid;
  auto const walked = RoadWalker::Walk(grid, At(60, 0), 90, -100);
  TEST(walked, ());
  TestAt(0, 0, walked->m_position);
  TEST_ALMOST_EQUAL_ABS(walked->m_bearingDeg, 90.0, 1.0, ());
  TEST_ALMOST_EQUAL_ABS(walked->m_appliedM, -60.0, 0.5, ());
  TEST(walked->m_atCrossing, ());
}

UNIT_TEST(NoGps_RoadWalker_LeavesCrossingItStandsAt)
{
  GridRoads grid;
  auto const walked = RoadWalker::Walk(grid, At(100, 0), 90, 30);
  TEST(walked, ());
  TestAt(130, 0, walked->m_position);
  TEST(!walked->m_atCrossing, ());
}

UNIT_TEST(NoGps_RoadWalker_FollowsRoadTheCarLooksAlong)
{
  // The car looks north on the north street, the east one crossing it is ignored.
  GridRoads grid;
  auto const walked = RoadWalker::Walk(grid, At(0, 10), 0, 50);
  TEST(walked, ());
  TestAt(0, 60, walked->m_position);
  TEST_ALMOST_EQUAL_ABS(walked->m_bearingDeg, 0.0, 1.0, ());
}

UNIT_TEST(NoGps_RoadWalker_StopsAtRoadEnd)
{
  GridRoads grid;
  auto const walked = RoadWalker::Walk(grid, At(490, 0), 90, 50);
  TEST(walked, ());
  TEST_ALMOST_EQUAL_ABS(walked->m_appliedM, 10.0, 5.1, ());
}

UNIT_TEST(NoGps_RoadWalker_NoRoad)
{
  GridRoads grid;
  TEST(!RoadWalker::Walk(grid, At(50, 50), 90, 50), ());
}

UNIT_TEST(NoGps_Geo_OrientsRoadTheCarWay)
{
  TEST_EQUAL(Orient(90, 250), 270, ());
  TEST_EQUAL(Orient(90, 100), 90, ());
  TEST_EQUAL(Orient(270, 100), 90, ());
  TEST_EQUAL(Orient(90, {}), 90, ());
}

UNIT_TEST(NoGps_TurnMatcher_NotFartherThanCalculationMayBeWrong)
{
  GridRoads grid;
  // The car turned north 40 m before the crossing by the calculation, which may be wrong by 20 m: that
  // crossing is too far, the car has turned somewhere else, e.g. into a yard.
  std::string whyNot;
  TEST(!TurnMatcher::FindCrossing(grid, At(160, 0), 90, 0, 20, &whyNot), ());
  TEST(whyNot.find("crossings around 0,") != std::string::npos, (whyNot));
  // 22 m before the crossing is within the least distance searched.
  TEST(TurnMatcher::FindCrossing(grid, At(178, 0), 90, 0, 20), ());
  TEST(TurnMatcher::FindCrossing(grid, At(160, 0), 90, 0, 45), ());
}

UNIT_TEST(NoGps_TurnMatcher_NotFartherThanCalculationIsEverWrong)
{
  // The street going east has one crossing, at 100 m. After kilometers without a known place the calculation
  // tells it may be wrong by 140 m, but it never is: a crossing 70 m away is not where the car has turned.
  PieceRoads roads({{-300, 0, 300, 0}, {100, -300, 100, 300}}, {{100, 0}});
  auto const crossing = TurnMatcher::FindCrossing(roads, At(45, 0), 90, 0, 140);
  TEST(crossing, ());
  TestAt(100, 0, *crossing);
  TEST(!TurnMatcher::FindCrossing(roads, At(30, 0), 90, 0, 140), ());
}

UNIT_TEST(NoGps_TurnMatcher_DescribesCrossingsAround)
{
  // The car going east has turned north at 160 m: the crossings of its street are 40 m ahead and 60 m behind.
  GridRoads grid;
  TEST_EQUAL(TurnMatcher::DescribeCrossings(grid, At(160, 0), 90, 0), "+40/+0/BA,-60/+0/BA", ());
  // A street with one crossing, the side street goes south only: no road going north there.
  PieceRoads roads({{-300, 0, 300, 0}, {100, -300, 100, 0}}, {{100, 0}});
  TEST_EQUAL(TurnMatcher::DescribeCrossings(roads, At(80, 5), 90, 0), "+20/+5/B-", ());
  TEST_EQUAL(TurnMatcher::DescribeCrossings(roads, At(-100, 0), 90, 0), "-", ());
}

UNIT_TEST(NoGps_TurnMatcher_DoesNotGuessAmongCrossings)
{
  // The car going south between two streets going that way, 15 m from each, turns east. A street going east
  // crosses both 13 m ahead.
  PieceRoads two({{-15, 50, -15, -100}, {15, 50, 15, -100}, {-200, -13, 200, -13}}, {{-15, -13}, {15, -13}});
  TEST(TurnMatcher::FindCrossing(two, At(0, 0), 180, 90, 25), ());
  std::string why;
  TEST(!TurnMatcher::FindCrossing(two, At(0, 0), 180, 90, 25, &why, true /* onlyOne */), ());
  TEST_EQUAL(why, "2 crossings fit, and the car was not on a road to tell which", ());

  // A crossing without a street going south is not one of them.
  PieceRoads one({{15, 50, 15, -100}, {-200, -13, 200, -13}}, {{-15, -13}, {15, -13}});
  auto crossing = TurnMatcher::FindCrossing(one, At(0, 0), 180, 90, 25, &why, true /* onlyOne */);
  TEST(crossing, (why));
  TEST_ALMOST_EQUAL_ABS(East(*crossing), 15.0, 0.1, ());

  // Nor is another crossing of the same junction, e.g. of the two carriageways of the street going east.
  PieceRoads junction({{15, 50, 15, -100}, {-200, -13, 200, -13}, {-200, -19, 200, -19}}, {{15, -13}, {15, -19}});
  crossing = TurnMatcher::FindCrossing(junction, At(0, 0), 180, 90, 25, &why, true /* onlyOne */);
  TEST(crossing, (why));
  TEST_ALMOST_EQUAL_ABS(North(*crossing), -13.0, 0.1, ());
}

UNIT_TEST(NoGps_TurnMatcher_MatchesTurnBeforeCrossing)
{
  // The calculated distance lags: the car turned left at 200 m, but it is at 160 m by the calculation.
  GridRoads grid;
  auto const crossing = TurnMatcher::FindCrossing(grid, At(160, 0), 90, 0, 60);
  TEST(crossing, ());
  TestAt(200, 0, *crossing);
}

UNIT_TEST(NoGps_TurnMatcher_MatchesTurnAfterCrossing)
{
  GridRoads grid;
  auto const crossing = TurnMatcher::FindCrossing(grid, At(240, 0), 90, 180, 60);
  TEST(crossing, ());
  TestAt(200, 0, *crossing);
}

UNIT_TEST(NoGps_TurnMatcher_IgnoresBends)
{
  GridRoads grid;
  std::string whyNot;
  TEST(!TurnMatcher::FindCrossing(grid, At(200, 0), 90, 70, 60, &whyNot), ());
  TEST_EQUAL(whyNot, "a turn that small is a bend of the road", ());
}

UNIT_TEST(NoGps_TurnMatcher_IgnoresTurnsAround)
{
  GridRoads grid;
  TEST(!TurnMatcher::FindCrossing(grid, At(200, 0), 90, 270, 60), ());
}

UNIT_TEST(NoGps_TurnMatcher_IgnoresParallelStreets)
{
  // A crossing 30 m aside is on a parallel street, not on the street the car has driven along.
  GridRoads grid;
  std::string whyNot;
  TEST(!TurnMatcher::FindCrossing(grid, At(190, 30), 90, 0, 60, &whyNot), ());
  TEST(whyNot.find("along the road or aside of it 1,") != std::string::npos, (whyNot));
}

UNIT_TEST(NoGps_TurnMatcher_NoCrossingOutsideRoads)
{
  GridRoads grid;
  TEST(!TurnMatcher::FindCrossing(grid, At(700, 0), 90, 0, 60), ());
}

UNIT_TEST(NoGps_TurnMatcher_IgnoresBendWithSideStreet)
{
  // The road going east bends to the south-east by 40 degrees, a side street leaves it 30 m before the bend the
  // same way. The car has followed the road.
  double const side = math::DegToRad(140.0);
  double const bend = math::DegToRad(110.0);
  double const after = math::DegToRad(130.0);
  PieceRoads roads(
      {
          {-200, 0, 0, 0},
          {0, 0, 40 * std::sin(bend), 40 * std::cos(bend)},
          {37.6, -13.7, 37.6 + 100 * std::sin(after), -13.7 + 100 * std::cos(after)},
          {-30, 0, -30 + 100 * std::sin(side), 100 * std::cos(side)},
      },
      {{-30, 0}});
  std::string whyNot;
  TEST(!TurnMatcher::FindCrossing(roads, At(0, 0), 90, 130, 60, &whyNot), ());
  TEST_EQUAL(whyNot, "the road bends that way", ());
  // The car has turned to the side street: the road it was on goes on east.
  PieceRoads straight(
      {
          {-200, 0, 200, 0},
          {-30, 0, -30 + 100 * std::sin(side), 100 * std::cos(side)},
      },
      {{-30, 0}});
  auto const crossing = TurnMatcher::FindCrossing(straight, At(0, 0), 90, 135, 60);
  TEST(crossing, ());
  TestAt(-30, 0, *crossing);
}
}  // namespace nogps_roads_tests
