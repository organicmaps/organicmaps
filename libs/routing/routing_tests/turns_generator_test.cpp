#include "testing/testing.hpp"

#include "routing/routing_tests/tools.hpp"

#include "routing/car_directions.hpp"
#include "routing/loaded_path_segment.hpp"
#include "routing/route.hpp"
#include "routing/routing_result_graph.hpp"
#include "routing/turns.hpp"
#include "routing/turns_generator.hpp"
#include "routing/turns_generator_utils.hpp"

#include "indexer/ftypes_matcher.hpp"

#include "platform/location.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"
#include "geometry/point_with_altitude.hpp"

#include "base/macros.hpp"

#include <string>
#include <vector>

namespace turn_generator_test
{
using namespace routing;
using namespace std;
using namespace turns;

// It's a dummy class to wrap |segments| for tests.
class RoutingResultTest : public IRoutingResult
{
public:
  explicit RoutingResultTest(TUnpackedPathSegments const & segments) : m_segments(segments) {}

  TUnpackedPathSegments const & GetSegments() const override { return m_segments; }

  void GetPossibleTurns(SegmentRange const & segmentRange, m2::PointD const & junctionPoint, size_t & ingoingCount,
                        TurnCandidates & outgoingTurns) const override
  {
    outgoingTurns.candidates.emplace_back(0.0, Segment(), ftypes::HighwayClass::Tertiary, false);
    outgoingTurns.isCandidatesAngleValid = false;
  }

  double GetPathLength() const override
  {
    NOTIMPLEMENTED();
    return 0.0;
  }

  geometry::PointWithAltitude GetStartPoint() const override
  {
    NOTIMPLEMENTED();
    return geometry::PointWithAltitude();
  }

  geometry::PointWithAltitude GetEndPoint() const override
  {
    NOTIMPLEMENTED();
    return geometry::PointWithAltitude();
  }

private:
  TUnpackedPathSegments m_segments;
};

UNIT_TEST(ApproachLanesFollowShortSplitsWithoutCrossingTurns)
{
  using namespace turns::lanes;

  vector<m2::PointD> const points = {mercator::FromLatLon(0.0, 0.0), mercator::FromLatLon(0.0, 0.00009),
                                     mercator::FromLatLon(0.0, 0.00018), mercator::FromLatLon(0.0, 0.00027)};
  auto const makeLanes = [](size_t count)
  {
    LanesInfo lanes(count - 1, {{LaneWay::Through}});
    lanes.push_back({{LaneWay::Right}});
    return lanes;
  };

  TUnpackedPathSegments loadedSegments(3);
  for (size_t i = 0; i < loadedSegments.size(); ++i)
  {
    auto & segment = loadedSegments[i];
    segment.m_path = {{points[i], geometry::kDefaultAltitudeMeters}, {points[i + 1], geometry::kDefaultAltitudeMeters}};
    segment.m_segments = {{0, 0, static_cast<uint32_t>(i), true}};
    segment.m_roadNameInfo = RouteSegment::RoadNameInfo("Main");
    segment.m_lanes = makeLanes(6 - i);
  }

  TurnItem turn(3, CarDirection::TurnRight);
  turn.m_lanes = loadedSegments.back().m_lanes;
  vector<RouteSegment> routeSegments;
  RouteSegmentsFrom({}, points, {{1, CarDirection::None}, {2, CarDirection::None}, turn}, {}, routeSegments);
  AddApproachLanes(loadedSegments, 2, routeSegments, turn);
  TEST_EQUAL(turn.m_approachLanes.size(), 2, ());
  TEST_EQUAL(turn.m_approachLanes[0].m_splitIndex, 2, ());
  TEST_EQUAL(turn.m_approachLanes[0].m_offset, 1, ());
  TEST_EQUAL(turn.m_approachLanes[0].m_lanes, makeLanes(5), ());
  TEST_EQUAL(turn.m_approachLanes[1].m_splitIndex, 1, ());
  TEST_EQUAL(turn.m_approachLanes[1].m_offset, 1, ());
  TEST_EQUAL(turn.m_approachLanes[1].m_lanes, makeLanes(6), ());
  TEST_EQUAL(turn.m_approachLanesBeginIndex, 0, ());

  routeSegments.clear();
  RouteSegmentsFrom({}, points, {{1, CarDirection::None}, {2, CarDirection::None}, turn}, {}, routeSegments);
  FixupCarTurns(routeSegments);
  Route route;
  route.SetRoutingSettings(GetRoutingSettings(VehicleType::Car));
  route.SetGeometry(points.begin(), points.end());
  route.SetRouteSegments(std::move(routeSegments));

  auto const getGps = [](m2::PointD const & point)
  {
    location::GpsInfo gps;
    gps.m_latitude = mercator::YToLat(point.y);
    gps.m_longitude = mercator::XToLon(point.x);
    gps.m_horizontalAccuracy = 2;
    return gps;
  };

  double distance = 0.0;
  TurnItem nextTurn;
  for (size_t i = 0; i < 3; ++i)
  {
    auto const point = (points[i] + points[i + 1]) / 2;
    TEST(route.MoveIterator(getGps(point)), ());
    route.GetNearestTurn(distance, nextTurn);
    TEST_EQUAL(nextTurn.m_lanes.size(), 6 - i, (i));
    TEST_EQUAL(nextTurn.m_lanes.back().recommendedWay, LaneWay::Right, (i));
  }

  // The two short segments together exceed the limit, so only one split is retained.
  loadedSegments[1].m_path = {{points[0], geometry::kDefaultAltitudeMeters},
                              {points[2], geometry::kDefaultAltitudeMeters}};
  loadedSegments[2].m_path = {{points[1], geometry::kDefaultAltitudeMeters},
                              {points[3], geometry::kDefaultAltitudeMeters}};
  turn.m_approachLanes.clear();
  routeSegments.clear();
  RouteSegmentsFrom({}, points, {{1, CarDirection::None}, {2, CarDirection::None}, turn}, {}, routeSegments);
  AddApproachLanes(loadedSegments, 2, routeSegments, turn);
  TEST_EQUAL(turn.m_approachLanes.size(), 1, ());
  loadedSegments[1].m_path = {{points[1], geometry::kDefaultAltitudeMeters},
                              {points[2], geometry::kDefaultAltitudeMeters}};
  loadedSegments[2].m_path = {{points[2], geometry::kDefaultAltitudeMeters},
                              {points[3], geometry::kDefaultAltitudeMeters}};

  // A previous turn only one short segment away blocks the lookup.
  routeSegments.clear();
  RouteSegmentsFrom({}, points, {{1, CarDirection::None}, {2, CarDirection::TurnLeft}, turn}, {}, routeSegments);
  turn.m_approachLanes.clear();
  AddApproachLanes(loadedSegments, 2, routeSegments, turn);
  TEST(turn.m_approachLanes.empty(), ());

  // A change of road blocks it even if there was no explicit maneuver.
  loadedSegments[1].m_roadNameInfo = RouteSegment::RoadNameInfo("Side");
  routeSegments.clear();
  RouteSegmentsFrom({}, points, {{1, CarDirection::None}, {2, CarDirection::None}, turn}, {}, routeSegments);
  AddApproachLanes(loadedSegments, 2, routeSegments, turn);
  TEST(turn.m_approachLanes.empty(), ());

  // An older segment across the road boundary must not inherit the five-lane layout.
  loadedSegments[1].m_roadNameInfo = RouteSegment::RoadNameInfo("Main");
  loadedSegments[0].m_roadNameInfo = RouteSegment::RoadNameInfo("Side");
  AddApproachLanes(loadedSegments, 2, routeSegments, turn);
  TEST_EQUAL(turn.m_approachLanes.size(), 1, ());
  TEST_EQUAL(turn.m_approachLanesBeginIndex, 1, ());

  routeSegments.clear();
  RouteSegmentsFrom({}, points, {{1, CarDirection::None}, {2, CarDirection::None}, turn}, {}, routeSegments);
  FixupCarTurns(routeSegments);
  Route boundedRoute;
  boundedRoute.SetRoutingSettings(GetRoutingSettings(VehicleType::Car));
  boundedRoute.SetGeometry(points.begin(), points.end());
  boundedRoute.SetRouteSegments(std::move(routeSegments));
  auto const beforeBoundary = (points[0] + points[1]) / 2;
  TEST(boundedRoute.MoveIterator(getGps(beforeBoundary)), ());
  boundedRoute.GetNearestTurn(distance, nextTurn);
  TEST_EQUAL(nextTurn.m_lanes.size(), 4, ());
  auto const afterBoundary = (points[1] + points[2]) / 2;
  TEST(boundedRoute.MoveIterator(getGps(afterBoundary)), ());
  boundedRoute.GetNearestTurn(distance, nextTurn);
  TEST_EQUAL(nextTurn.m_lanes.size(), 5, ());
}

UNIT_TEST(ApproachLanesContinueAcrossIdenticalLayouts)
{
  using namespace turns::lanes;

  vector<m2::PointD> const points = {mercator::FromLatLon(0.0, 0.0), mercator::FromLatLon(0.0, 0.00090),
                                     mercator::FromLatLon(0.0, 0.00099), mercator::FromLatLon(0.0, 0.00108)};
  auto const makeLanes = [](size_t count)
  {
    LanesInfo lanes(count - 1, {{LaneWay::Through}});
    lanes.push_back({{LaneWay::Right}});
    return lanes;
  };

  TUnpackedPathSegments loadedSegments(3);
  for (size_t i = 0; i < loadedSegments.size(); ++i)
  {
    auto & segment = loadedSegments[i];
    segment.m_path = {{points[i], geometry::kDefaultAltitudeMeters}, {points[i + 1], geometry::kDefaultAltitudeMeters}};
    segment.m_segments = {{0, 0, static_cast<uint32_t>(i), true}};
    segment.m_roadNameInfo = RouteSegment::RoadNameInfo("Main");
  }

  vector<RouteSegment> routeSegments;
  RouteSegmentsFrom({}, points, {{1, CarDirection::None}, {2, CarDirection::None}, {3, CarDirection::TurnRight}}, {},
                    routeSegments);

  auto const checkDisplayedLanes = [&](TurnItem const & turn, vector<size_t> const & counts)
  {
    vector<RouteSegment> segments;
    RouteSegmentsFrom({}, points, {{1, CarDirection::None}, {2, CarDirection::None}, turn}, {}, segments);
    FixupCarTurns(segments);
    Route route;
    route.SetRoutingSettings(GetRoutingSettings(VehicleType::Car));
    route.SetGeometry(points.begin(), points.end());
    route.SetRouteSegments(std::move(segments));

    for (size_t i = 0; i < counts.size(); ++i)
    {
      auto const point = (points[i] + points[i + 1]) / 2;
      location::GpsInfo gps;
      gps.m_latitude = mercator::YToLat(point.y);
      gps.m_longitude = mercator::XToLon(point.x);
      gps.m_horizontalAccuracy = 2;
      TEST(route.MoveIterator(gps), (i));
      double distance;
      TurnItem nextTurn;
      route.GetNearestTurn(distance, nextTurn);
      TEST_EQUAL(nextTurn.m_lanes.size(), counts[i], (i));
    }
  };

  loadedSegments[0].m_lanes = makeLanes(6);
  loadedSegments[1].m_lanes = makeLanes(6);
  loadedSegments[2].m_lanes = makeLanes(5);
  TurnItem turn(3, CarDirection::TurnRight);
  turn.m_lanes = loadedSegments[2].m_lanes;
  AddApproachLanes(loadedSegments, 2, routeSegments, turn);
  TEST_EQUAL(turn.m_approachLanes.size(), 1, ());
  TEST_EQUAL(turn.m_approachLanes[0].m_splitIndex, 2, ());
  TEST_EQUAL(turn.m_approachLanesBeginIndex, 0, ());
  checkDisplayedLanes(turn, {6, 6, 5});

  loadedSegments[1].m_lanes = makeLanes(5);
  turn = TurnItem(3, CarDirection::TurnRight);
  turn.m_lanes = loadedSegments[2].m_lanes;
  AddApproachLanes(loadedSegments, 2, routeSegments, turn);
  TEST_EQUAL(turn.m_approachLanes.size(), 1, ());
  TEST_EQUAL(turn.m_approachLanes[0].m_splitIndex, 1, ());
  TEST_EQUAL(turn.m_approachLanesBeginIndex, 0, ());
  checkDisplayedLanes(turn, {6, 5, 5});

  // A 40 m part after the split is too long to borrow the earlier layout.
  loadedSegments[2].m_path.back() = {mercator::FromLatLon(0.0, 0.00135), geometry::kDefaultAltitudeMeters};
  turn = TurnItem(3, CarDirection::TurnRight);
  turn.m_lanes = loadedSegments[2].m_lanes;
  AddApproachLanes(loadedSegments, 2, routeSegments, turn);
  TEST(turn.m_approachLanes.empty(), ());
}

UNIT_TEST(ApproachLanesRecommendOnlySurvivingLane)
{
  using namespace turns::lanes;

  vector<m2::PointD> const points = {mercator::FromLatLon(0.0, 0.0), mercator::FromLatLon(0.0, 0.00009),
                                     mercator::FromLatLon(0.0, 0.00018)};
  TUnpackedPathSegments loadedSegments(2);
  for (size_t i = 0; i < loadedSegments.size(); ++i)
  {
    auto & segment = loadedSegments[i];
    segment.m_path = {{points[i], geometry::kDefaultAltitudeMeters}, {points[i + 1], geometry::kDefaultAltitudeMeters}};
    segment.m_segments = {{0, 0, static_cast<uint32_t>(i), true}};
    segment.m_roadNameInfo = RouteSegment::RoadNameInfo("Main");
  }

  loadedSegments[0].m_lanes = {{{LaneWay::Left}}, {{LaneWay::Left}}, {{LaneWay::Through}}};
  loadedSegments[1].m_lanes = {{{LaneWay::Left}}, {{LaneWay::Through}}};
  TurnItem turn(2, CarDirection::TurnLeft);
  turn.m_lanes = loadedSegments[1].m_lanes;
  vector<RouteSegment> routeSegments;
  RouteSegmentsFrom({}, points, {{1, CarDirection::None}, turn}, {}, routeSegments);
  AddApproachLanes(loadedSegments, 1, routeSegments, turn);
  TEST_EQUAL(turn.m_approachLanes.size(), 1, ());
  TEST_EQUAL(turn.m_approachLanes[0].m_offset, 1, ());

  routeSegments.clear();
  RouteSegmentsFrom({}, points, {{1, CarDirection::None}, turn}, {}, routeSegments);
  FixupCarTurns(routeSegments);
  auto const & approach = routeSegments.back().GetTurn().m_approachLanes[0].m_lanes;
  TEST_EQUAL(approach[0].recommendedWay, LaneWay::None, ());
  TEST_EQUAL(approach[1].recommendedWay, LaneWay::Left, ());
  TEST_EQUAL(approach[2].recommendedWay, LaneWay::None, ());

  // Two identical matches cannot identify which lane stays on the route.
  loadedSegments[0].m_lanes = {{{LaneWay::Left}}, {{LaneWay::Left}}};
  loadedSegments[1].m_lanes = {{{LaneWay::Left}}};
  turn = TurnItem(2, CarDirection::TurnLeft);
  turn.m_lanes = loadedSegments[1].m_lanes;
  AddApproachLanes(loadedSegments, 1, routeSegments, turn);
  TEST(turn.m_approachLanes.empty(), ());
}

UNIT_TEST(TestFixupTurns)
{
  double const kHalfSquareSideMeters = 10.;
  m2::PointD const kSquareCenterLonLat = {0., 0.};
  m2::RectD const kSquareNearZero =
      mercator::MetersToXY(kSquareCenterLonLat.x, kSquareCenterLonLat.y, kHalfSquareSideMeters);
  {
    // Removing a turn in case staying on a roundabout.
    vector<m2::PointD> const pointsMerc1 = {{kSquareNearZero.minX(), kSquareNearZero.minY()},
                                            {kSquareNearZero.minX(), kSquareNearZero.minY()},
                                            {kSquareNearZero.maxX(), kSquareNearZero.maxY()},
                                            {kSquareNearZero.maxX(), kSquareNearZero.minY()}};
    // The constructor TurnItem(uint32_t idx, CarDirection t, uint32_t exitNum = 0)
    // is used for initialization of vector<TurnItem> below.
    vector<turns::TurnItem> turnsDir1 = {
        {1, CarDirection::EnterRoundAbout}, {2, CarDirection::StayOnRoundAbout}, {3, CarDirection::LeaveRoundAbout}};
    vector<RouteSegment> routeSegments;
    RouteSegmentsFrom({}, pointsMerc1, turnsDir1, {}, routeSegments);
    FixupCarTurns(routeSegments);
    vector<turns::TurnItem> const expectedTurnDir1 = {
        {1, CarDirection::EnterRoundAbout, 2}, {2, CarDirection::None, 0}, {3, CarDirection::LeaveRoundAbout, 2}};
    TEST_EQUAL(routeSegments[0].GetTurn(), expectedTurnDir1[0], ());
    TEST_EQUAL(routeSegments[1].GetTurn(), expectedTurnDir1[1], ());
    TEST_EQUAL(routeSegments[2].GetTurn(), expectedTurnDir1[2], ());
  }
  {
    // Merging turns which are close to each other.
    vector<m2::PointD> const pointsMerc2 = {{kSquareNearZero.minX(), kSquareNearZero.minY()},
                                            {kSquareNearZero.minX(), kSquareNearZero.minY()},
                                            {kSquareCenterLonLat.x, kSquareCenterLonLat.y},
                                            {kSquareNearZero.maxX(), kSquareNearZero.maxY()}};
    vector<turns::TurnItem> turnsDir2 = {
        {1, CarDirection::None}, {2, CarDirection::GoStraight}, {3, CarDirection::TurnLeft}};
    vector<RouteSegment> routeSegments2;
    RouteSegmentsFrom({}, pointsMerc2, turnsDir2, {}, routeSegments2);
    FixupCarTurns(routeSegments2);
    vector<turns::TurnItem> const expectedTurnDir2 = {
        {1, CarDirection::None}, {2, CarDirection::None}, {3, CarDirection::TurnLeft}};
    TEST_EQUAL(routeSegments2[0].GetTurn(), expectedTurnDir2[0], ());
    TEST_EQUAL(routeSegments2[1].GetTurn(), expectedTurnDir2[1], ());
    TEST_EQUAL(routeSegments2[2].GetTurn(), expectedTurnDir2[2], ());
  }
  {
    // No turn is removed.
    vector<m2::PointD> const pointsMerc3 = {
        {kSquareNearZero.minX(), kSquareNearZero.minY()},
        {kSquareNearZero.minX(), kSquareNearZero.maxY()},
        {kSquareNearZero.maxX(), kSquareNearZero.maxY()},
    };
    vector<turns::TurnItem> turnsDir3 = {{1, CarDirection::None}, {2, CarDirection::TurnRight}};

    vector<RouteSegment> routeSegments3;
    RouteSegmentsFrom({}, {}, turnsDir3, {}, routeSegments3);
    FixupCarTurns(routeSegments3);
    vector<turns::TurnItem> const expectedTurnDir3 = {{1, CarDirection::None}, {2, CarDirection::TurnRight}};

    TEST_EQUAL(routeSegments3[0].GetTurn(), expectedTurnDir3[0], ());
    TEST_EQUAL(routeSegments3[1].GetTurn(), expectedTurnDir3[1], ());
  }
}

UNIT_TEST(TestGetRoundaboutDirection)
{
  // The signature of GetRoundaboutDirection function is
  // GetRoundaboutDirection(bool isIngoingEdgeRoundabout, bool isOutgoingEdgeRoundabout,
  //     bool isMultiTurnJunction, bool keepTurnByHighwayClass)
  TEST_EQUAL(GetRoundaboutDirectionBasic(true, true, true, true), CarDirection::StayOnRoundAbout, ());
  TEST_EQUAL(GetRoundaboutDirectionBasic(true, true, true, false), CarDirection::None, ());
  TEST_EQUAL(GetRoundaboutDirectionBasic(true, true, false, true), CarDirection::None, ());
  TEST_EQUAL(GetRoundaboutDirectionBasic(true, true, false, false), CarDirection::None, ());
  TEST_EQUAL(GetRoundaboutDirectionBasic(false, true, false, true), CarDirection::EnterRoundAbout, ());
  TEST_EQUAL(GetRoundaboutDirectionBasic(true, false, false, false), CarDirection::LeaveRoundAbout, ());
}

UNIT_TEST(TestInvertDirection)
{
  TEST_EQUAL(InvertDirection(CarDirection::TurnSlightRight), CarDirection::TurnSlightLeft, ());
  TEST_EQUAL(InvertDirection(CarDirection::TurnRight), CarDirection::TurnLeft, ());
  TEST_EQUAL(InvertDirection(CarDirection::TurnSharpRight), CarDirection::TurnSharpLeft, ());
  TEST_EQUAL(InvertDirection(CarDirection::TurnSlightLeft), CarDirection::TurnSlightRight, ());
  TEST_EQUAL(InvertDirection(CarDirection::TurnSlightRight), CarDirection::TurnSlightLeft, ());
  TEST_EQUAL(InvertDirection(CarDirection::TurnLeft), CarDirection::TurnRight, ());
  TEST_EQUAL(InvertDirection(CarDirection::TurnSharpLeft), CarDirection::TurnSharpRight, ());
}

UNIT_TEST(TestRightmostDirection)
{
  TEST_EQUAL(RightmostDirection(180.), CarDirection::TurnSharpRight, ());
  TEST_EQUAL(RightmostDirection(170.), CarDirection::TurnSharpRight, ());
  TEST_EQUAL(RightmostDirection(90.), CarDirection::TurnRight, ());
  TEST_EQUAL(RightmostDirection(45.), CarDirection::TurnSlightRight, ());
  TEST_EQUAL(RightmostDirection(0.), CarDirection::GoStraight, ());
  TEST_EQUAL(RightmostDirection(-20.), CarDirection::GoStraight, ());
  TEST_EQUAL(RightmostDirection(-90.), CarDirection::GoStraight, ());
  TEST_EQUAL(RightmostDirection(-170.), CarDirection::GoStraight, ());
}

UNIT_TEST(TestLeftmostDirection)
{
  TEST_EQUAL(LeftmostDirection(180.), CarDirection::GoStraight, ());
  TEST_EQUAL(LeftmostDirection(170.), CarDirection::GoStraight, ());
  TEST_EQUAL(LeftmostDirection(90.), CarDirection::GoStraight, ());
  TEST_EQUAL(LeftmostDirection(45.), CarDirection::GoStraight, ());
  TEST_EQUAL(LeftmostDirection(0.), CarDirection::GoStraight, ());
  TEST_EQUAL(LeftmostDirection(-20.), CarDirection::TurnSlightLeft, ());
  TEST_EQUAL(LeftmostDirection(-90.), CarDirection::TurnLeft, ());
  TEST_EQUAL(LeftmostDirection(-170.), CarDirection::TurnSharpLeft, ());
}

UNIT_TEST(TestIntermediateDirection)
{
  TEST_EQUAL(IntermediateDirection(180.), CarDirection::TurnSharpRight, ());
  TEST_EQUAL(IntermediateDirection(170.), CarDirection::TurnSharpRight, ());
  TEST_EQUAL(IntermediateDirection(90.), CarDirection::TurnRight, ());
  TEST_EQUAL(IntermediateDirection(45.), CarDirection::TurnSlightRight, ());
  TEST_EQUAL(IntermediateDirection(0.), CarDirection::GoStraight, ());
  TEST_EQUAL(IntermediateDirection(-20.), CarDirection::TurnSlightLeft, ());
  TEST_EQUAL(IntermediateDirection(-90.), CarDirection::TurnLeft, ());
  TEST_EQUAL(IntermediateDirection(-170.), CarDirection::TurnSharpLeft, ());
}

UNIT_TEST(TestCheckUTurnOnRoute)
{
  TUnpackedPathSegments pathSegments(4, LoadedPathSegment());
  pathSegments[0].m_roadNameInfo = {"A road"};
  pathSegments[0].m_highwayClass = ftypes::HighwayClass::Trunk;
  pathSegments[0].m_onRoundabout = false;
  pathSegments[0].m_isLink = false;
  pathSegments[0].m_path = {{{0, 0}, 0}, {{0, 1}, 0}};
  pathSegments[0].m_segmentRange =
      SegmentRange(FeatureID(), 0 /* start seg id */, 1 /* end seg id */, true /* forward */,
                   pathSegments[0].m_path.front().GetPoint(), pathSegments[0].m_path.back().GetPoint());

  pathSegments[1] = pathSegments[0];
  pathSegments[1].m_segmentRange =
      SegmentRange(FeatureID(), 1 /* start seg id */, 2 /* end seg id */, true /* forward */,
                   pathSegments[1].m_path.front().GetPoint(), pathSegments[1].m_path.back().GetPoint());
  pathSegments[1].m_path = {{{0, 1}, 0}, {{0, 0}, 0}};

  pathSegments[2] = pathSegments[0];
  pathSegments[2].m_segmentRange =
      SegmentRange(FeatureID(), 2 /* start seg id */, 3 /* end seg id */, true /* forward */,
                   pathSegments[2].m_path.front().GetPoint(), pathSegments[2].m_path.back().GetPoint());
  pathSegments[2].m_path = {{{0, 0}, 0}, {{0, 1}, 0}};

  pathSegments[3] = pathSegments[0];
  pathSegments[3].m_segmentRange =
      SegmentRange(FeatureID(), 3 /* start seg id */, 4 /* end seg id */, true /* forward */,
                   pathSegments[3].m_path.front().GetPoint(), pathSegments[3].m_path.back().GetPoint());
  pathSegments[3].m_path.clear();

  RoutingResultTest resultTest(pathSegments);
  RoutingSettings const vehicleSettings = GetRoutingSettings(VehicleType::Car);
  // Zigzag test.
  TurnItem turn1;
  TEST_EQUAL(CheckUTurnOnRoute(resultTest, 1 /* outgoingSegmentIndex */, NumMwmIds(), vehicleSettings, turn1), 1, ());
  TEST_EQUAL(turn1.m_turn, CarDirection::UTurnLeft, ());
  TurnItem turn2;
  TEST_EQUAL(CheckUTurnOnRoute(resultTest, 2 /* outgoingSegmentIndex */, NumMwmIds(), vehicleSettings, turn2), 1, ());
  TEST_EQUAL(turn2.m_turn, CarDirection::UTurnLeft, ());

  // Empty path test.
  TurnItem turn3;
  TEST_EQUAL(CheckUTurnOnRoute(resultTest, 3 /* outgoingSegmentIndex */, NumMwmIds(), vehicleSettings, turn3), 0, ());
}

// GetPointForTurn() must not overshoot its limits on sparse geometry: when one long geometry
// edge crosses the time limit, a point on this edge at the limit is expected instead of its
// far end. Otherwise the turn angle gets the road curvature accumulated far from the junction.
// See https://github.com/organicmaps/organicmaps/issues/13152
UNIT_TEST(GetPointForTurnOnSparseGeometry)
{
  // For HighwayClass::LivingStreet (20 km/h) the 3 seconds limit of GetPointForTurn()
  // is reached in 20 / 3.6 * 3 ~= 16.67 meters.
  double constexpr kExpectedDistM = 20.0 / 3.6 * 3.0;
  double constexpr kEpsM = 0.2;

  m2::PointD const junction = mercator::FromLatLon(0.0, 0.0);

  TUnpackedPathSegments pathSegments(2, LoadedPathSegment());
  pathSegments[0].m_highwayClass = ftypes::HighwayClass::LivingStreet;
  pathSegments[0].m_path = {
      {mercator::GetSmPoint(junction, -105.0, 0.0), 0}, {mercator::GetSmPoint(junction, -5.0, 0.0), 0}, {junction, 0}};
  pathSegments[1].m_highwayClass = ftypes::HighwayClass::LivingStreet;
  pathSegments[1].m_path = {
      {junction, 0}, {mercator::GetSmPoint(junction, 5.0, 0.0), 0}, {mercator::GetSmPoint(junction, 105.0, 0.0), 0}};

  RoutingResultTest resultTest(pathSegments);
  RoutingSettings const vehicleSettings = GetRoutingSettings(VehicleType::Car);

  // Forward: a 5 m edge, then a 100 m edge which overshoots the limit.
  m2::PointD const outgoingPoint =
      GetPointForTurn(resultTest, 1 /* outgoingSegmentIndex */, NumMwmIds(), vehicleSettings.m_maxOutgoingPointsCount,
                      vehicleSettings.m_minOutgoingDistMeters, true /* forward */);
  TEST_ALMOST_EQUAL_ABS(mercator::DistanceOnEarth(junction, outgoingPoint), kExpectedDistM, kEpsM, ());

  // Backward: the same, in the ingoing direction.
  m2::PointD const ingoingPoint =
      GetPointForTurn(resultTest, 1 /* outgoingSegmentIndex */, NumMwmIds(), vehicleSettings.m_maxIngoingPointsCount,
                      vehicleSettings.m_minIngoingDistMeters, false /* forward */);
  TEST_ALMOST_EQUAL_ABS(mercator::DistanceOnEarth(junction, ingoingPoint), kExpectedDistM, kEpsM, ());
}

UNIT_TEST(GetNextRoutePointIndex)
{
  TUnpackedPathSegments pathSegments(2, LoadedPathSegment());
  pathSegments[0].m_path = {{{0, 0}, 0}, {{0, 1}, 0}, {{0, 2}, 0}};
  pathSegments[1].m_path = {{{0, 2}, 0}, {{1, 2}, 0}};

  RoutingResultTest resultTest(pathSegments);
  RoutePointIndex nextIndex;

  // Forward direction.
  TEST(GetNextRoutePointIndex(resultTest, RoutePointIndex({0 /* m_segmentIndex */, 0 /* m_pathIndex */}), NumMwmIds(),
                              true /* forward */, nextIndex),
       ());
  TEST_EQUAL(nextIndex, RoutePointIndex({0 /* m_segmentIndex */, 1 /* m_pathIndex */}), ());

  TEST(GetNextRoutePointIndex(resultTest, RoutePointIndex({0 /* m_segmentIndex */, 1 /* m_pathIndex */}), NumMwmIds(),
                              true /* forward */, nextIndex),
       ());
  TEST_EQUAL(nextIndex, RoutePointIndex({0 /* m_segmentIndex */, 2 /* m_pathIndex */}), ());

  // Trying to get next item after the last item of the first segment.
  // False because of too sharp turn angle.
  TEST(!GetNextRoutePointIndex(resultTest, RoutePointIndex({0 /* m_segmentIndex */, 2 /* m_pathIndex */}), NumMwmIds(),
                               true /* forward */, nextIndex),
       ());

  // Trying to get point about the end of the route.
  TEST(!GetNextRoutePointIndex(resultTest, RoutePointIndex({1 /* m_segmentIndex */, 1 /* m_pathIndex */}), NumMwmIds(),
                               true /* forward */, nextIndex),
       ());

  // Backward direction.
  // Moving in backward direction it's possible to get index of the first item of a segment.
  TEST(GetNextRoutePointIndex(resultTest, RoutePointIndex({1 /* m_segmentIndex */, 1 /* m_pathIndex */}), NumMwmIds(),
                              false /* forward */, nextIndex),
       ());
  TEST_EQUAL(nextIndex, RoutePointIndex({1 /* m_segmentIndex */, 0 /* m_pathIndex */}), ());

  TEST(GetNextRoutePointIndex(resultTest, RoutePointIndex({0 /* m_segmentIndex */, 2 /* m_pathIndex */}), NumMwmIds(),
                              false /* forward */, nextIndex),
       ());
  TEST_EQUAL(nextIndex, RoutePointIndex({0 /* m_segmentIndex */, 1 /* m_pathIndex */}), ());

  TEST(GetNextRoutePointIndex(resultTest, RoutePointIndex({0 /* m_segmentIndex */, 1 /* m_pathIndex */}), NumMwmIds(),
                              false /* forward */, nextIndex),
       ());
  TEST_EQUAL(nextIndex, RoutePointIndex({0 /* m_segmentIndex */, 0 /* m_pathIndex */}), ());

  // Trying to get point before the beginning.
  TEST(!GetNextRoutePointIndex(resultTest, RoutePointIndex({0 /* m_segmentIndex */, 0 /* m_pathIndex */}), NumMwmIds(),
                               false /* forward */, nextIndex),
       ());
}
}  // namespace turn_generator_test
