#include "testing/testing.hpp"

#include "routing/route.hpp"
#include "routing/routing_helpers.hpp"
#include "routing/turns.hpp"

#include "routing/routing_tests/tools.hpp"

#include "platform/location.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"
#include "geometry/point_with_altitude.hpp"

#include <string>
#include <vector>

namespace route_tests
{
using namespace routing;
using namespace std;

// For all test geometry: geometry[0] == geometry[1], since info about 1st point will be lost.
static vector<m2::PointD> const kTestGeometry = {{0, 0}, {0, 0}, {1, 1}, {1, 2}, {1, 3}, {1, 4}};
static vector<Segment> const kTestSegments = {
    {0, 0, 0, true}, {0, 0, 1, true}, {0, 0, 2, true}, {0, 0, 3, true}, {0, 0, 4, true}};
static vector<turns::TurnItem> const kTestTurns({turns::TurnItem(1, turns::CarDirection::None),
                                                 turns::TurnItem(2, turns::CarDirection::TurnLeft),
                                                 turns::TurnItem(3, turns::CarDirection::TurnRight),
                                                 turns::TurnItem(4, turns::CarDirection::None),
                                                 turns::TurnItem(5, turns::CarDirection::ReachedYourDestination)});
static vector<double> const kTestTimes = {0.0, 7.0, 10.0, 19.0, 20.0};
static vector<RouteSegment::RoadNameInfo> const kTestNames = {{"Street0"}, {"Street1"}, {"Street2"}, {""}, {"Street3"}};

void GetTestRouteSegments(vector<m2::PointD> const & routePoints, vector<turns::TurnItem> const & turns,
                          vector<RouteSegment::RoadNameInfo> const & streets, vector<double> const & times,
                          vector<RouteSegment> & routeSegments)
{
  RouteSegmentsFrom({}, routePoints, turns, streets, routeSegments);
  FillSegmentInfo(kTestTimes, routeSegments);
}

location::GpsInfo GetGps(double x, double y)
{
  location::GpsInfo info;
  info.m_latitude = mercator::YToLat(y);
  info.m_longitude = mercator::XToLon(x);
  info.m_horizontalAccuracy = 2;
  return info;
}

vector<vector<Segment>> const GetSegments()
{
  auto const segmentsAllReal = kTestSegments;
  vector<Segment> const segmentsAllFake = {{kFakeNumMwmId, 0, 0, true},
                                           {kFakeNumMwmId, 0, 1, true},
                                           {kFakeNumMwmId, 0, 2, true},
                                           {kFakeNumMwmId, 0, 3, true},
                                           {kFakeNumMwmId, 0, 4, true}};
  vector<Segment> const segmentsFakeHeadAndTail = {
      {kFakeNumMwmId, 0, 0, true}, {0, 0, 1, true}, {0, 0, 2, true}, {0, 0, 3, true}, {kFakeNumMwmId, 0, 4, true}};
  return {segmentsAllReal, segmentsFakeHeadAndTail, segmentsAllFake};
}

UNIT_TEST(FinshRouteOnSomeDistanceToTheFinishPointTest)
{
  for (auto const vehicleType : {VehicleType::Car, VehicleType::Bicycle, VehicleType::Pedestrian, VehicleType::Transit})
  {
    auto const settings = GetRoutingSettings(vehicleType);
    for (auto const & segments : GetSegments())
    {
      Route route;
      route.SetRoutingSettings(settings);

      vector<RouteSegment> routeSegments;
      RouteSegmentsFrom(segments, kTestGeometry, kTestTurns, {}, routeSegments);
      FillSegmentInfo(kTestTimes, routeSegments);
      route.SetRouteSegments(std::move(routeSegments));

      route.SetGeometry(kTestGeometry.begin(), kTestGeometry.end());
      route.SetSubroutes(vector<Route::SubrouteAttrs>(
          {Route::SubrouteAttrs(geometry::PointWithAltitude(kTestGeometry.front(), geometry::kDefaultAltitudeMeters),
                                geometry::PointWithAltitude(kTestGeometry.back(), geometry::kDefaultAltitudeMeters), 0,
                                kTestSegments.size())}));

      // The route should be finished at some distance to the finish point.
      double const distToFinish = settings.m_finishToleranceM;

      route.MoveIterator(GetGps(kTestGeometry.back().x, kTestGeometry.back().y - 0.1));
      TEST(!route.IsSubroutePassed(0), ());
      TEST_GREATER(route.GetCurrentDistanceToEndMeters(), distToFinish, ());

      route.MoveIterator(GetGps(kTestGeometry.back().x, kTestGeometry.back().y - 0.02));
      TEST(!route.IsSubroutePassed(0), ());
      TEST_GREATER(route.GetCurrentDistanceToEndMeters(), distToFinish, ());

      // Finish tolerance value for cars is greater then for other vehicle types.
      // The iterator for other types should be moved closer to the finish point.
      if (vehicleType == VehicleType::Car)
        route.MoveIterator(GetGps(kTestGeometry.back().x, kTestGeometry.back().y - 0.00014));
      else
        route.MoveIterator(GetGps(kTestGeometry.back().x, kTestGeometry.back().y - 0.00011));

      TEST(route.IsSubroutePassed(0), ());
      TEST_LESS(route.GetCurrentDistanceToEndMeters(), distToFinish, ());
    }
  }
}

UNIT_TEST(DistanceAndTimeToCurrentTurnTest)
{
  // |curTurn.m_index| is an index of the point of |curTurn| at polyline |route.m_poly|.

  Route route;
  vector<RouteSegment> routeSegments;
  GetTestRouteSegments(kTestGeometry, kTestTurns, {}, kTestTimes, routeSegments);
  route.SetGeometry(kTestGeometry.begin(), kTestGeometry.end());
  vector<turns::TurnItem> turns(kTestTurns);

  route.SetRouteSegments(std::move(routeSegments));

  double distance;
  turns::TurnItem turn;

  {
    // Initial point.
    auto pos = kTestGeometry[0];

    route.GetNearestTurn(distance, turn);
    size_t currentTurnIndex = 2;  // Turn with m_index == 1 is None.
    TEST_ALMOST_EQUAL_ABS(distance, mercator::DistanceOnEarth(pos, kTestGeometry[currentTurnIndex]), 0.1, ());
    TEST_EQUAL(turn, kTestTurns[currentTurnIndex - 1], ());

    double timePassed = 0;

    double time = route.GetCurrentTimeToEndSec();
    TEST_ALMOST_EQUAL_ABS(time, kTestTimes[4] - timePassed, 0.1, ());

    time = route.GetCurrentTimeToNearestTurnSec();
    TEST_ALMOST_EQUAL_ABS(time, kTestTimes[currentTurnIndex - 1] - timePassed, 0.1, ());
  }
  {
    // Move between points 1 and 2.
    auto const pos = (kTestGeometry[1] + kTestGeometry[2]) / 2;
    route.MoveIterator(GetGps(pos.x, pos.y));

    route.GetNearestTurn(distance, turn);
    size_t const currentTurnIndex = 2;
    TEST_ALMOST_EQUAL_ABS(distance, mercator::DistanceOnEarth(pos, kTestGeometry[currentTurnIndex]), 0.1, ());
    TEST_EQUAL(turn, kTestTurns[currentTurnIndex - 1], ());

    double const timePassed = (kTestTimes[1 - 1] + kTestTimes[2 - 1]) / 2;

    double time = route.GetCurrentTimeToEndSec();
    TEST_ALMOST_EQUAL_ABS(time, kTestTimes[4] - timePassed, 0.1, ());

    time = route.GetCurrentTimeToNearestTurnSec();
    TEST_ALMOST_EQUAL_ABS(time, kTestTimes[currentTurnIndex - 1] - timePassed, 0.1, ());
  }
  {
    // Move between points 2 and 3.
    auto const pos = kTestGeometry[2] * 0.8 + kTestGeometry[3] * 0.2;
    route.MoveIterator(GetGps(pos.x, pos.y));

    route.GetNearestTurn(distance, turn);
    size_t const currentTurnIndex = 3;
    TEST_ALMOST_EQUAL_ABS(distance, mercator::DistanceOnEarth(pos, kTestGeometry[currentTurnIndex]), 0.1, ());
    TEST_EQUAL(turn, kTestTurns[currentTurnIndex - 1], ());

    double const timePassed = 0.8 * kTestTimes[2 - 1] + 0.2 * kTestTimes[3 - 1];

    double time = route.GetCurrentTimeToEndSec();
    TEST_ALMOST_EQUAL_ABS(time, kTestTimes[4] - timePassed, 0.1, ());

    time = route.GetCurrentTimeToNearestTurnSec();
    TEST_ALMOST_EQUAL_ABS(time, kTestTimes[currentTurnIndex - 1] - timePassed, 0.1, ());
  }
  {
    // Move between points 3 and 4.
    auto const pos = kTestGeometry[3] * 0.3 + kTestGeometry[4] * 0.7;
    route.MoveIterator(GetGps(pos.x, pos.y));

    route.GetNearestTurn(distance, turn);
    size_t const currentTurnIndex = 5;  // Turn with m_index == 4 is None.
    TEST_ALMOST_EQUAL_ABS(distance, mercator::DistanceOnEarth(pos, kTestGeometry[currentTurnIndex]), 0.1, ());
    TEST_EQUAL(turn, kTestTurns[currentTurnIndex - 1], ());

    double const timePassed = 0.3 * kTestTimes[3 - 1] + 0.7 * kTestTimes[4 - 1];

    double time = route.GetCurrentTimeToEndSec();
    TEST_ALMOST_EQUAL_ABS(time, kTestTimes[4] - timePassed, 0.1, ());

    time = route.GetCurrentTimeToNearestTurnSec();
    TEST_ALMOST_EQUAL_ABS(time, kTestTimes[currentTurnIndex - 1] - timePassed, 0.1, ());
  }
}

UNIT_TEST(NextTurnTest)
{
  Route route;
  vector<RouteSegment> routeSegments;
  GetTestRouteSegments(kTestGeometry, kTestTurns, {}, {}, routeSegments);
  route.SetRouteSegments(std::move(routeSegments));
  route.SetGeometry(kTestGeometry.begin(), kTestGeometry.end());

  double distance, nextDistance;
  turns::TurnItem turn, nextTurn;

  {
    // Initial point.
    size_t const currentTurnIndex = 2;  // Turn with m_index == 1 is None.
    route.GetNearestTurn(distance, turn);
    TEST_EQUAL(turn, kTestTurns[currentTurnIndex - 1], ());

    size_t const nextTurnIndex = 3;
    route.GetNextTurn(nextDistance, nextTurn);
    TEST_EQUAL(nextTurn, kTestTurns[nextTurnIndex - 1], ());
  }
  {
    // Move between points 1 and 2.
    auto const pos = (kTestGeometry[1] + kTestGeometry[2]) / 2;
    route.MoveIterator(GetGps(pos.x, pos.y));

    size_t const currentTurnIndex = 2;
    route.GetNearestTurn(distance, turn);
    TEST_EQUAL(turn, kTestTurns[currentTurnIndex - 1], ());

    size_t const nextTurnIndex = 3;
    route.GetNextTurn(nextDistance, nextTurn);
    TEST_EQUAL(nextTurn, kTestTurns[nextTurnIndex - 1], ());
  }
  {
    // Move between points 3 and 4.
    auto const pos = (kTestGeometry[3] + kTestGeometry[4]) / 2;
    route.MoveIterator(GetGps(pos.x, pos.y));

    size_t const currentTurnIndex = 5;  // Turn with m_index == 4 is None.
    route.GetNearestTurn(distance, turn);
    TEST_EQUAL(turn, kTestTurns[currentTurnIndex - 1], ());

    // nextTurn is absent.
    route.GetNextTurn(nextDistance, nextTurn);
    TEST_EQUAL(nextTurn, turns::TurnItem(), ());
  }
}

UNIT_TEST(NextTurnsTest)
{
  Route route;
  route.SetGeometry(kTestGeometry.begin(), kTestGeometry.end());
  vector<RouteSegment> routeSegments;
  GetTestRouteSegments(kTestGeometry, kTestTurns, {}, {}, routeSegments);
  route.SetRouteSegments(std::move(routeSegments));

  vector<turns::TurnItem> turns(kTestTurns);
  vector<turns::TurnItemDist> turnsDist;

  {
    // Initial point.
    auto const pos = kTestGeometry[0];

    size_t const currentTurnIndex = 2;  // Turn with m_index == 1 is None.
    size_t const nextTurnIndex = 3;
    TEST(route.GetNextTurns(turnsDist), ());
    TEST_EQUAL(turnsDist.size(), 2, ());
    double const firstSegLenM = mercator::DistanceOnEarth(pos, kTestGeometry[currentTurnIndex]);
    double const secondSegLenM =
        mercator::DistanceOnEarth(kTestGeometry[currentTurnIndex], kTestGeometry[nextTurnIndex]);
    TEST_EQUAL(turnsDist[0].m_turnItem, kTestTurns[currentTurnIndex - 1], ());
    TEST_EQUAL(turnsDist[1].m_turnItem, kTestTurns[nextTurnIndex - 1], ());
    TEST_ALMOST_EQUAL_ABS(turnsDist[0].m_distMeters, firstSegLenM, 0.1, ());
    TEST_ALMOST_EQUAL_ABS(turnsDist[1].m_distMeters, firstSegLenM + secondSegLenM, 0.1, ());
  }
  {
    // Move between points 1 and 2.
    auto const pos = (kTestGeometry[1] + kTestGeometry[2]) / 2;
    route.MoveIterator(GetGps(pos.x, pos.y));

    size_t const currentTurnIndex = 2;
    size_t const nextTurnIndex = 3;
    TEST(route.GetNextTurns(turnsDist), ());
    TEST_EQUAL(turnsDist.size(), 2, ());
    double const firstSegLenM = mercator::DistanceOnEarth(pos, kTestGeometry[currentTurnIndex]);
    double const secondSegLenM =
        mercator::DistanceOnEarth(kTestGeometry[currentTurnIndex], kTestGeometry[nextTurnIndex]);
    TEST_EQUAL(turnsDist[0].m_turnItem, kTestTurns[currentTurnIndex - 1], ());
    TEST_EQUAL(turnsDist[1].m_turnItem, kTestTurns[nextTurnIndex - 1], ());
    TEST_ALMOST_EQUAL_ABS(turnsDist[0].m_distMeters, firstSegLenM, 0.1, ());
    TEST_ALMOST_EQUAL_ABS(turnsDist[1].m_distMeters, firstSegLenM + secondSegLenM, 0.1, ());
  }
  {
    // Move between points 2 and 3.
    auto const pos = (kTestGeometry[2] + kTestGeometry[3]) / 2;
    route.MoveIterator(GetGps(pos.x, pos.y));

    size_t const currentTurnIndex = 3;
    size_t const nextTurnIndex = 5;  // Turn with m_index == 4 is None.
    TEST(route.GetNextTurns(turnsDist), ());
    TEST_EQUAL(turnsDist.size(), 2, ());
    double const firstSegLenM = mercator::DistanceOnEarth(pos, kTestGeometry[currentTurnIndex]);
    double const secondSegLenM =
        mercator::DistanceOnEarth(kTestGeometry[currentTurnIndex], kTestGeometry[nextTurnIndex]);
    TEST_EQUAL(turnsDist[0].m_turnItem, kTestTurns[currentTurnIndex - 1], ());
    TEST_EQUAL(turnsDist[1].m_turnItem, kTestTurns[nextTurnIndex - 1], ());
    TEST_ALMOST_EQUAL_ABS(turnsDist[0].m_distMeters, firstSegLenM, 0.1, ());
    TEST_ALMOST_EQUAL_ABS(turnsDist[1].m_distMeters, firstSegLenM + secondSegLenM, 0.1, ());
  }
  {
    // Move between points 3 and 4.
    auto const pos = (kTestGeometry[3] + kTestGeometry[4]) / 2;
    route.MoveIterator(GetGps(pos.x, pos.y));

    size_t const currentTurnIndex = 5;  // Turn with m_index == 4 is None.
    // nextTurn is absent.
    TEST(route.GetNextTurns(turnsDist), ());
    double const firstSegLenM = mercator::DistanceOnEarth(pos, kTestGeometry[currentTurnIndex]);
    TEST_EQUAL(turnsDist[0].m_turnItem, kTestTurns[currentTurnIndex - 1], ());
    TEST_ALMOST_EQUAL_ABS(turnsDist[0].m_distMeters, firstSegLenM, 0.1, ());
  }
}

//   0.0002          *--------*
//                   |        |
//                   |        |
//        Finish     |        |
//   0.0001  *----------------*
//                   |
//                   |
//                   |
//        0          * Start
//           0  0.0001    0.0002
//
UNIT_TEST(SelfIntersectedRouteMatchingTest)
{
  vector<m2::PointD> const kRouteGeometry = {{0.0001, 0.0},    {0.0001, 0.0},    {0.0001, 0.0002},
                                             {0.0002, 0.0002}, {0.0002, 0.0001}, {0.0, 0.0001}};
  double constexpr kRoundingErrorMeters = 0.001;

  Route route;
  route.SetGeometry(kRouteGeometry.begin(), kRouteGeometry.end());

  vector<RouteSegment> routeSegments;
  GetTestRouteSegments(kRouteGeometry, {}, {}, {}, routeSegments);
  route.SetRouteSegments(std::move(routeSegments));

  auto const testMachedPos =
      [&](location::GpsInfo const & pos, location::GpsInfo const & expectedMatchingPos, size_t expectedIndexInRoute)
  {
    location::RouteMatchingInfo routeMatchingInfo;
    route.MoveIterator(pos);
    location::GpsInfo matchedPos = pos;
    route.MatchLocationToRoute(matchedPos, routeMatchingInfo);
    TEST_LESS(mercator::DistanceOnEarth(m2::PointD(matchedPos.m_latitude, matchedPos.m_longitude),
                                        m2::PointD(expectedMatchingPos.m_latitude, expectedMatchingPos.m_longitude)),
              kRoundingErrorMeters, ());
    TEST_EQUAL(max(size_t(1), routeMatchingInfo.GetIndexInRoute()), expectedIndexInRoute, ());
  };

  // Moving along the route from start point.
  location::GpsInfo const pos1(GetGps(0.0001, 0.0));
  testMachedPos(pos1, pos1, 1 /* expectedIndexInRoute */);
  location::GpsInfo const pos2(GetGps(0.0001, 0.00005));
  testMachedPos(pos2, pos2, 1 /* expectedIndexInRoute */);

  // Moving around the self intersection and checking that position is matched to the first segment of the route.
  location::GpsInfo const selfIntersectionPos(GetGps(0.0001, 0.0001));
  location::GpsInfo const pos3(GetGps(0.00005, 0.0001));
  testMachedPos(pos3, selfIntersectionPos, 1 /* expectedIndexInRoute */);
  location::GpsInfo const pos4(GetGps(0.00015, 0.0001));
  testMachedPos(pos4, selfIntersectionPos, 1 /* expectedIndexInRoute */);

  // Continue moving along the route.
  location::GpsInfo const pos5(GetGps(0.00011, 0.0002));
  testMachedPos(pos5, pos5, 2 /* expectedIndexInRoute */);
  location::GpsInfo const pos6(GetGps(0.0002, 0.00019));
  testMachedPos(pos6, pos6, 3 /* expectedIndexInRoute */);
  location::GpsInfo const pos7(GetGps(0.00019, 0.0001));
  testMachedPos(pos7, pos7, 4 /* expectedIndexInRoute */);

  // Moving around the self intersection and checking that position is matched to the last segment of the route.
  location::GpsInfo const pos8(GetGps(0.0001, 0.00005));
  testMachedPos(pos8, selfIntersectionPos, 4 /* expectedIndexInRoute */);
  location::GpsInfo const pos9(GetGps(0.0001, 0.00015));
  testMachedPos(pos9, selfIntersectionPos, 4 /* expectedIndexInRoute */);
}

UNIT_TEST(RouteNameTest)
{
  Route route;

  route.SetGeometry(kTestGeometry.begin(), kTestGeometry.end());
  vector<RouteSegment> routeSegments;
  GetTestRouteSegments(kTestGeometry, kTestTurns, kTestNames, {}, routeSegments);
  route.SetRouteSegments(std::move(routeSegments));

  RouteSegment::RoadNameInfo roadNameInfo;
  route.GetCurrentStreetName(roadNameInfo);
  TEST_EQUAL(roadNameInfo.m_name, "Street0", (roadNameInfo.m_name));

  route.GetClosestStreetNameAfterIdx(1, roadNameInfo);
  TEST_EQUAL(roadNameInfo.m_name, "Street1", (roadNameInfo.m_name));

  route.GetClosestStreetNameAfterIdx(2, roadNameInfo);
  TEST_EQUAL(roadNameInfo.m_name, "Street2", (roadNameInfo.m_name));

  route.GetClosestStreetNameAfterIdx(3, roadNameInfo);
  TEST_EQUAL(roadNameInfo.m_name, "Street3", (roadNameInfo.m_name));

  route.GetClosestStreetNameAfterIdx(4, roadNameInfo);
  TEST_EQUAL(roadNameInfo.m_name, "Street3", (roadNameInfo.m_name));

  location::GpsInfo const pos(GetGps(1.0, 2.0));
  route.MoveIterator(pos);
  route.GetCurrentStreetName(roadNameInfo);
  TEST_EQUAL(roadNameInfo.m_name, "Street2", (roadNameInfo.m_name));
}

// Builds a Route whose junctions sit at the given path points along the equator. |path[0]| must
// equal |path[1]| per FillSegmentInfo's convention (segment 0 carries dist=0). One Segment per
// (path.size()-1).
Route MakeRoute(vector<m2::PointD> const & path, vector<Segment> const & segments)
{
  CHECK_EQUAL(path.size(), segments.size() + 1, ());
  Route r;
  vector<RouteSegment> segs;
  RouteSegmentsFrom(segments, path, {}, {}, segs);
  FillSegmentInfo(vector<double>(segments.size(), 0.0), segs);
  r.SetRouteSegments(std::move(segs));
  r.SetSubroutes(vector<Route::SubrouteAttrs>{Route::SubrouteAttrs(
      geometry::PointWithAltitude(path.front(), geometry::kDefaultAltitudeMeters),
      geometry::PointWithAltitude(path.back(), geometry::kDefaultAltitudeMeters), 0, segments.size())});
  return r;
}

// 10 unit-length segments along the equator with junctions at (0..9, 0). The first FillSegmentInfo
// segment carries dist=0 (the path convention duplicates path[0]==path[1]) so the total geodesic
// length is 9 deg of longitude. The midpoint by distance lands at the (4.5, 0) interpolation
// between junctions (4, 0) and (5, 0).
UNIT_TEST(RouteBase_GetMidpoint_Whole)
{
  vector<m2::PointD> path = {{0, 0}, {0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0}, {8, 0}, {9, 0}};
  vector<Segment> segs;
  for (uint32_t i = 0; i < 10; ++i)
    segs.emplace_back(0, 0, i, true);

  auto const route = MakeRoute(path, segs);
  auto const mid = route.GetMidpoint();
  TEST_ALMOST_EQUAL_ABS(mid.x, 4.5, 1e-6, (mid));
  TEST_ALMOST_EQUAL_ABS(mid.y, 0.0, 1e-6, (mid));
}

// Subrange [3, 6] covers segments whose junctions span x = 3..6 (with the run start at segment 3's
// "previous junction" = (2, 0)). Midpoint by distance falls at (4, 0).
UNIT_TEST(RouteBase_GetMidpoint_Subrange)
{
  vector<m2::PointD> path = {{0, 0}, {0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0}, {8, 0}, {9, 0}};
  vector<Segment> segs;
  for (uint32_t i = 0; i < 10; ++i)
    segs.emplace_back(0, 0, i, true);

  auto const route = MakeRoute(path, segs);
  auto const mid = route.GetMidpoint(3, 6);
  TEST_ALMOST_EQUAL_ABS(mid.x, 4.0, 1e-6, (mid));
  TEST_ALMOST_EQUAL_ABS(mid.y, 0.0, 1e-6, (mid));
}

// Same featureId set on both sides → every segment matches → no divergence → nullopt.
UNIT_TEST(RouteBase_FindMaxDiffMidpoint_NoDiff)
{
  vector<m2::PointD> path = {{0, 0}, {0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}};
  vector<Segment> segs;
  for (uint32_t i = 0; i < 5; ++i)
    segs.emplace_back(0, 0, i, true);

  auto const route = MakeRoute(path, segs);
  TEST(!route.FindMaxDiffMidpoint(route.GetRouteSegments()).has_value(), ());
}

// 30-segment route; only segment 15 diverges → ~1/29 ≈ 3.4% of total length, below the 5% gate
// that FindMaxDiffMidpoint applies. The single diverging segment is real (not fake), so the
// rejection is purely on the threshold.
UNIT_TEST(RouteBase_FindMaxDiffMidpoint_BelowThreshold)
{
  size_t constexpr kN = 30;
  vector<m2::PointD> path;
  path.reserve(kN + 1);
  path.push_back({0, 0});
  for (size_t i = 0; i < kN; ++i)
    path.push_back({static_cast<double>(i), 0});
  vector<Segment> originSegs;
  vector<Segment> altSegs;
  for (uint32_t i = 0; i < kN; ++i)
  {
    originSegs.emplace_back(0, 0, i, true);
    altSegs.emplace_back(0, i == kN / 2 ? 1 : 0, i, true);
  }
  auto const altRoute = MakeRoute(path, altSegs);
  auto const originRoute = MakeRoute(path, originSegs);

  TEST(!altRoute.FindMaxDiffMidpoint(originRoute.GetRouteSegments()).has_value(), ());
}

// 10 segments; segments [3..6] (4 of them) carry featureId 1 instead of 0. Diff fraction = 4/9 ≈
// 44%, well above the 5% gate. The single diff run is [3, 6], whose midpoint by distance lands at
// x = 4 (start dist = seg[2].dist = 2, end dist = seg[6].dist = 6, mid = 4).
UNIT_TEST(RouteBase_FindMaxDiffMidpoint_AboveThreshold)
{
  vector<m2::PointD> path = {{0, 0}, {0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0}, {8, 0}, {9, 0}};
  vector<Segment> originSegs;
  vector<Segment> altSegs;
  for (uint32_t i = 0; i < 10; ++i)
  {
    originSegs.emplace_back(0, 0, i, true);
    altSegs.emplace_back(0, (i >= 3 && i <= 6) ? 1 : 0, i, true);
  }
  auto const altRoute = MakeRoute(path, altSegs);
  auto const originRoute = MakeRoute(path, originSegs);

  auto const mid = altRoute.FindMaxDiffMidpoint(originRoute.GetRouteSegments());
  TEST(mid.has_value(), ());
  TEST_ALMOST_EQUAL_ABS(mid->x, 4.0, 1e-6, (*mid));
  TEST_ALMOST_EQUAL_ABS(mid->y, 0.0, 1e-6, (*mid));
}
}  // namespace route_tests
