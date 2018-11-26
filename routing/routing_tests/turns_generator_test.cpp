#include "testing/testing.hpp"

#include "routing/loaded_path_segment.hpp"
#include "routing/route.hpp"
#include "routing/routing_result_graph.hpp"
#include "routing/turns.hpp"
#include "routing/turns_generator.hpp"

#include "indexer/ftypes_matcher.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "base/macros.hpp"

#include "std/cmath.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

using namespace routing;
using namespace turns;

namespace
{
// It's a dummy class to wrap |segments| for tests.
class RoutingResultTest : public IRoutingResult
{
public:
  explicit RoutingResultTest(TUnpackedPathSegments const & segments) : m_segments(segments) {}

  TUnpackedPathSegments const & GetSegments() const override { return m_segments; }

  void GetPossibleTurns(SegmentRange const & segmentRange, m2::PointD const & junctionPoint,
                        size_t & ingoingCount, TurnCandidates & outgoingTurns) const override
  {
    outgoingTurns.candidates.emplace_back(0.0, Segment(), ftypes::HighwayClass::Tertiary, false);
    outgoingTurns.isCandidatesAngleValid = false;
  }

  double GetPathLength() const override
  {
    NOTIMPLEMENTED();
    return 0.0;
  }

  Junction GetStartPoint() const override
  {
    NOTIMPLEMENTED();
    return Junction();
  }

  Junction GetEndPoint() const override
  {
    NOTIMPLEMENTED();
    return Junction();
  }

private:
  TUnpackedPathSegments m_segments;
};

UNIT_TEST(TestSplitLanes)
{
  vector<string> result;
  SplitLanes("through|through|through|through;right", '|', result);
  vector<string> const expected1 = {"through", "through", "through", "through;right"};
  TEST_EQUAL(result, expected1, ());

  SplitLanes("adsjkddfasui8747&sxdsdlad8\"\'", '|', result);
  TEST_EQUAL(result, vector<string>({"adsjkddfasui8747&sxdsdlad8\"\'"}), ());

  SplitLanes("|||||||", '|', result);
  vector<string> expected2 = {"", "", "", "", "", "", ""};
  TEST_EQUAL(result, expected2, ());
}

UNIT_TEST(TestParseSingleLane)
{
  TSingleLane result;
  TEST(ParseSingleLane("through;right", ';', result), ());
  TSingleLane const expected1 = {LaneWay::Through, LaneWay::Right};
  TEST_EQUAL(result, expected1, ());

  TEST(!ParseSingleLane("through;Right", ';', result), ());

  TEST(!ParseSingleLane("through ;right", ';', result), ());
  TEST_EQUAL(result.size(), 0, ());

  TEST(!ParseSingleLane("SD32kk*887;;", ';', result), ());
  TEST_EQUAL(result.size(), 0, ());

  TEST(!ParseSingleLane("Что-то на кириллице", ';', result), ());
  TEST_EQUAL(result.size(), 0, ());

  TEST(!ParseSingleLane("משהו בעברית", ';', result), ());
  TEST_EQUAL(result.size(), 0, ());

  TEST(ParseSingleLane("left;through", ';', result), ());
  TSingleLane expected2 = {LaneWay::Left, LaneWay::Through};
  TEST_EQUAL(result, expected2, ());

  TEST(ParseSingleLane("left", ';', result), ());
  TEST_EQUAL(result.size(), 1, ());
  TEST_EQUAL(result[0], LaneWay::Left, ());
}

UNIT_TEST(TestParseLanes)
{
  vector<SingleLaneInfo> result;
  TEST(ParseLanes("through|through|through|through;right", result), ());
  vector<SingleLaneInfo> const expected1 = {{LaneWay::Through},
                                            {LaneWay::Through},
                                            {LaneWay::Through},
                                            {LaneWay::Through, LaneWay::Right}};
  TEST_EQUAL(result, expected1, ());

  TEST(ParseLanes("left|left;through|through|through", result), ());
  vector<SingleLaneInfo> const expected2 = {
      {LaneWay::Left}, {LaneWay::Left, LaneWay::Through}, {LaneWay::Through}, {LaneWay::Through}};
  TEST_EQUAL(result, expected2, ());

  TEST(ParseLanes("left|through|through", result), ());
  vector<SingleLaneInfo> const expected3 = {
      {LaneWay::Left}, {LaneWay::Through}, {LaneWay::Through}};
  TEST_EQUAL(result, expected3, ());

  TEST(ParseLanes("left|le  ft|   through|through   |  right", result), ());
  vector<SingleLaneInfo> const expected4 = {
      {LaneWay::Left}, {LaneWay::Left}, {LaneWay::Through}, {LaneWay::Through}, {LaneWay::Right}};
  TEST_EQUAL(result, expected4, ());

  TEST(ParseLanes("left|Left|through|througH|right", result), ());
  vector<SingleLaneInfo> const expected5 = {
      {LaneWay::Left}, {LaneWay::Left}, {LaneWay::Through}, {LaneWay::Through}, {LaneWay::Right}};
  TEST_EQUAL(result, expected5, ());

  TEST(ParseLanes("left|Left|through|througH|through;right;sharp_rIght", result), ());
  vector<SingleLaneInfo> const expected6 = {
      {LaneWay::Left},
      {LaneWay::Left},
      {LaneWay::Through},
      {LaneWay::Through},
      {LaneWay::Through, LaneWay::Right, LaneWay::SharpRight}};
  TEST_EQUAL(result, expected6, ());

  TEST(!ParseLanes("left|Leftt|through|througH|right", result), ());
  TEST_EQUAL(result.size(), 0, ());

  TEST(!ParseLanes("Что-то на кириллице", result), ());
  TEST_EQUAL(result.size(), 0, ());

  TEST(!ParseLanes("משהו בעברית", result), ());
  TEST_EQUAL(result.size(), 0, ());

  TEST(ParseLanes("left |Left|through|througH|right", result), ());
  vector<SingleLaneInfo> const expected7 = {
      {LaneWay::Left}, {LaneWay::Left}, {LaneWay::Through}, {LaneWay::Through}, {LaneWay::Right}};
  TEST_EQUAL(result, expected7, ());
}

UNIT_TEST(TestFixupTurns)
{
  double const kHalfSquareSideMeters = 10.;
  m2::PointD const kSquareCenterLonLat = {0., 0.};
  m2::RectD const kSquareNearZero = MercatorBounds::MetersToXY(kSquareCenterLonLat.x,
                                                               kSquareCenterLonLat.y, kHalfSquareSideMeters);
  // Removing a turn in case staying on a roundabout.
  vector<Junction> const pointsMerc1 = {
    {{ kSquareNearZero.minX(), kSquareNearZero.minY() }, feature::kDefaultAltitudeMeters},
    {{ kSquareNearZero.minX(), kSquareNearZero.maxY() }, feature::kDefaultAltitudeMeters},
    {{ kSquareNearZero.maxX(), kSquareNearZero.maxY() }, feature::kDefaultAltitudeMeters},
    {{ kSquareNearZero.maxX(), kSquareNearZero.minY() }, feature::kDefaultAltitudeMeters},
  };
  // The constructor TurnItem(uint32_t idx, CarDirection t, uint32_t exitNum = 0)
  // is used for initialization of vector<TurnItem> below.
  Route::TTurns turnsDir1 = {{0, CarDirection::EnterRoundAbout},
                             {1, CarDirection::StayOnRoundAbout},
                             {2, CarDirection::LeaveRoundAbout},
                             {3, CarDirection::ReachedYourDestination}};

  FixupTurns(pointsMerc1, turnsDir1);
  Route::TTurns const expectedTurnDir1 = {{0, CarDirection::EnterRoundAbout, 2},
                                          {2, CarDirection::LeaveRoundAbout, 2},
                                          {3, CarDirection::ReachedYourDestination}};
  TEST_EQUAL(turnsDir1, expectedTurnDir1, ());

  // Merging turns which are close to each other.
  vector<Junction> const pointsMerc2 = {
    {{ kSquareNearZero.minX(), kSquareNearZero.minY()}, feature::kDefaultAltitudeMeters},
    {{ kSquareCenterLonLat.x, kSquareCenterLonLat.y }, feature::kDefaultAltitudeMeters},
    {{ kSquareNearZero.maxX(), kSquareNearZero.maxY() }, feature::kDefaultAltitudeMeters},
  };
  Route::TTurns turnsDir2 = {{0, CarDirection::GoStraight},
                             {1, CarDirection::TurnLeft},
                             {2, CarDirection::ReachedYourDestination}};

  FixupTurns(pointsMerc2, turnsDir2);
  Route::TTurns const expectedTurnDir2 = {{1, CarDirection::TurnLeft},
                                          {2, CarDirection::ReachedYourDestination}};
  TEST_EQUAL(turnsDir2, expectedTurnDir2, ());

  // No turn is removed.
  vector<Junction> const pointsMerc3 = {
    {{ kSquareNearZero.minX(), kSquareNearZero.minY()}, feature::kDefaultAltitudeMeters},
    {{ kSquareNearZero.minX(), kSquareNearZero.maxY() }, feature::kDefaultAltitudeMeters},
    {{ kSquareNearZero.maxX(), kSquareNearZero.maxY() }, feature::kDefaultAltitudeMeters},
  };
  Route::TTurns turnsDir3 = {{1, CarDirection::TurnRight},
                             {2, CarDirection::ReachedYourDestination}};

  FixupTurns(pointsMerc3, turnsDir3);
  Route::TTurns const expectedTurnDir3 = {{1, CarDirection::TurnRight},
                                          {2, CarDirection::ReachedYourDestination}};
  TEST_EQUAL(turnsDir3, expectedTurnDir3, ());
}

UNIT_TEST(TestIsLaneWayConformedTurnDirection)
{
  TEST(IsLaneWayConformedTurnDirection(LaneWay::Left, CarDirection::TurnLeft), ());
  TEST(IsLaneWayConformedTurnDirection(LaneWay::Right, CarDirection::TurnRight), ());
  TEST(IsLaneWayConformedTurnDirection(LaneWay::SlightLeft, CarDirection::TurnSlightLeft), ());
  TEST(IsLaneWayConformedTurnDirection(LaneWay::SharpRight, CarDirection::TurnSharpRight), ());
  TEST(IsLaneWayConformedTurnDirection(LaneWay::Reverse, CarDirection::UTurnLeft), ());
  TEST(IsLaneWayConformedTurnDirection(LaneWay::Reverse, CarDirection::UTurnRight), ());
  TEST(IsLaneWayConformedTurnDirection(LaneWay::Through, CarDirection::GoStraight), ());

  TEST(!IsLaneWayConformedTurnDirection(LaneWay::Left, CarDirection::TurnSlightLeft), ());
  TEST(!IsLaneWayConformedTurnDirection(LaneWay::Right, CarDirection::TurnSharpRight), ());
  TEST(!IsLaneWayConformedTurnDirection(LaneWay::SlightLeft, CarDirection::GoStraight), ());
  TEST(!IsLaneWayConformedTurnDirection(LaneWay::SharpRight, CarDirection::None), ());
  TEST(!IsLaneWayConformedTurnDirection(LaneWay::Reverse, CarDirection::TurnLeft), ());
  TEST(!IsLaneWayConformedTurnDirection(LaneWay::None, CarDirection::ReachedYourDestination), ());
}

UNIT_TEST(TestIsLaneWayConformedTurnDirectionApproximately)
{
  TEST(IsLaneWayConformedTurnDirectionApproximately(LaneWay::Left, CarDirection::TurnSharpLeft), ());
  TEST(IsLaneWayConformedTurnDirectionApproximately(LaneWay::Left, CarDirection::TurnSlightLeft), ());
  TEST(IsLaneWayConformedTurnDirectionApproximately(LaneWay::Right, CarDirection::TurnSharpRight), ());
  TEST(IsLaneWayConformedTurnDirectionApproximately(LaneWay::Right, CarDirection::TurnRight), ());
  TEST(IsLaneWayConformedTurnDirectionApproximately(LaneWay::Reverse, CarDirection::UTurnLeft), ());
  TEST(IsLaneWayConformedTurnDirectionApproximately(LaneWay::Reverse, CarDirection::UTurnRight), ());
  TEST(IsLaneWayConformedTurnDirectionApproximately(LaneWay::SlightLeft, CarDirection::GoStraight), ());
  TEST(IsLaneWayConformedTurnDirectionApproximately(LaneWay::SlightRight, CarDirection::GoStraight), ());

  TEST(!IsLaneWayConformedTurnDirectionApproximately(LaneWay::SharpLeft, CarDirection::UTurnLeft), ());
  TEST(!IsLaneWayConformedTurnDirectionApproximately(LaneWay::SharpLeft, CarDirection::UTurnRight), ());
  TEST(!IsLaneWayConformedTurnDirectionApproximately(LaneWay::SharpRight, CarDirection::UTurnLeft), ());
  TEST(!IsLaneWayConformedTurnDirectionApproximately(LaneWay::SharpRight, CarDirection::UTurnRight), ());
  TEST(!IsLaneWayConformedTurnDirection(LaneWay::Through, CarDirection::ReachedYourDestination), ());
  TEST(!IsLaneWayConformedTurnDirectionApproximately(LaneWay::Through, CarDirection::TurnRight), ());
  TEST(!IsLaneWayConformedTurnDirectionApproximately(LaneWay::SlightRight,
                                                     CarDirection::TurnSharpLeft), ());
}

UNIT_TEST(TestAddingActiveLaneInformation)
{
  Route::TTurns turns = {{0, CarDirection::GoStraight},
                         {1, CarDirection::TurnLeft},
                         {2, CarDirection::ReachedYourDestination}};
  turns[0].m_lanes.push_back({LaneWay::Left, LaneWay::Through});
  turns[0].m_lanes.push_back({LaneWay::Right});

  turns[1].m_lanes.push_back({LaneWay::SlightLeft});
  turns[1].m_lanes.push_back({LaneWay::Through});
  turns[1].m_lanes.push_back({LaneWay::Through});

  SelectRecommendedLanes(turns);

  TEST(turns[0].m_lanes[0].m_isRecommended, ());
  TEST(!turns[0].m_lanes[1].m_isRecommended, ());

  TEST(turns[1].m_lanes[0].m_isRecommended, ());
  TEST(!turns[1].m_lanes[1].m_isRecommended, ());
  TEST(!turns[1].m_lanes[1].m_isRecommended, ());
}

UNIT_TEST(TestGetRoundaboutDirection)
{
  // The signature of GetRoundaboutDirection function is
  // GetRoundaboutDirection(bool isIngoingEdgeRoundabout, bool isOutgoingEdgeRoundabout,
  //     bool isMultiTurnJunction, bool keepTurnByHighwayClass)
  TEST_EQUAL(GetRoundaboutDirection(true, true, true, true), CarDirection::StayOnRoundAbout, ());
  TEST_EQUAL(GetRoundaboutDirection(true, true, true, false), CarDirection::None, ());
  TEST_EQUAL(GetRoundaboutDirection(true, true, false, true), CarDirection::None, ());
  TEST_EQUAL(GetRoundaboutDirection(true, true, false, false), CarDirection::None, ());
  TEST_EQUAL(GetRoundaboutDirection(false, true, false, true), CarDirection::EnterRoundAbout, ());
  TEST_EQUAL(GetRoundaboutDirection(true, false, false, false), CarDirection::LeaveRoundAbout, ());
}

UNIT_TEST(TestCheckRoundaboutEntrance)
{
  // The signature of CheckRoundaboutEntrance function is
  // CheckRoundaboutEntrance(bool isIngoingEdgeRoundabout, bool isOutgoingEdgeRoundabout)
  TEST(!CheckRoundaboutEntrance(true, true), ());
  TEST(!CheckRoundaboutEntrance(false, false), ());
  TEST(!CheckRoundaboutEntrance(true, false), ());
  TEST(CheckRoundaboutEntrance(false, true), ());
}

UNIT_TEST(TestCheckRoundaboutExit)
{
  // The signature of GetRoundaboutDirection function is
  // CheckRoundaboutExit(bool isIngoingEdgeRoundabout, bool isOutgoingEdgeRoundabout)
  TEST(!CheckRoundaboutExit(true, true), ());
  TEST(!CheckRoundaboutExit(false, false), ());
  TEST(CheckRoundaboutExit(true, false), ());
  TEST(!CheckRoundaboutExit(false, true), ());
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
  TEST_EQUAL(RightmostDirection(-20.), CarDirection::TurnSlightLeft, ());
  TEST_EQUAL(RightmostDirection(-90.), CarDirection::TurnLeft, ());
  TEST_EQUAL(RightmostDirection(-170.), CarDirection::TurnSharpLeft, ());
}

UNIT_TEST(TestLeftmostDirection)
{
  TEST_EQUAL(LeftmostDirection(180.), CarDirection::TurnSharpRight, ());
  TEST_EQUAL(LeftmostDirection(170.), CarDirection::TurnSharpRight, ());
  TEST_EQUAL(LeftmostDirection(90.), CarDirection::TurnRight, ());
  TEST_EQUAL(LeftmostDirection(45.), CarDirection::TurnSlightRight, ());
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

UNIT_TEST(TestCalculateMercatorDistanceAlongRoute)
{
  vector<m2::PointD> const points = {{0., 0.}, {0., 1.}, {0., 1.}, {1., 1.}};

  uint32_t const lastPointIdx = static_cast<uint32_t>(points.size() - 1);
  TEST_EQUAL(CalculateMercatorDistanceAlongPath(0, lastPointIdx, points), 2., ());
  TEST_EQUAL(CalculateMercatorDistanceAlongPath(1, 1, points), 0., ());
  TEST_EQUAL(CalculateMercatorDistanceAlongPath(1, 2, points), 0., ());
  TEST_EQUAL(CalculateMercatorDistanceAlongPath(0, 1, points), 1., ());
}

UNIT_TEST(TestCheckUTurnOnRoute)
{
  TUnpackedPathSegments pathSegments(4, LoadedPathSegment());
  pathSegments[0].m_name = "A road";
  pathSegments[0].m_weight = 1;
  pathSegments[0].m_highwayClass = ftypes::HighwayClass::Trunk;
  pathSegments[0].m_onRoundabout = false;
  pathSegments[0].m_isLink = false;
  pathSegments[0].m_path = {{{0, 0}, 0}, {{0, 1}, 0}};
  pathSegments[0].m_segmentRange = SegmentRange(FeatureID(), 0 /* start seg id */, 1 /* end seg id */,
                                                true /* forward */,
                                                pathSegments[0].m_path.front().GetPoint(),
                                                pathSegments[0].m_path.back().GetPoint());

  pathSegments[1] = pathSegments[0];
  pathSegments[1].m_segmentRange = SegmentRange(FeatureID(), 1 /* start seg id */, 2 /* end seg id */,
                                     true /* forward */,
                                     pathSegments[1].m_path.front().GetPoint(),
                                     pathSegments[1].m_path.back().GetPoint());
  pathSegments[1].m_path = {{{0, 1}, 0}, {{0, 0}, 0}};

  pathSegments[2] = pathSegments[0];
  pathSegments[2].m_segmentRange = SegmentRange(FeatureID(), 2 /* start seg id */, 3 /* end seg id */,
                                       true /* forward */,
                                       pathSegments[2].m_path.front().GetPoint(),
                                       pathSegments[2].m_path.back().GetPoint());
  pathSegments[2].m_path = {{{0, 0}, 0}, {{0, 1}, 0}};

  pathSegments[3] = pathSegments[0];
  pathSegments[3].m_segmentRange = SegmentRange(FeatureID(), 3 /* start seg id */, 4 /* end seg id */,
                                       true /* forward */,
                                       pathSegments[3].m_path.front().GetPoint(),
                                       pathSegments[3].m_path.back().GetPoint());
  pathSegments[3].m_path.clear();

  RoutingResultTest resultTest(pathSegments);

  // Zigzag test.
  TurnItem turn1;
  TEST_EQUAL(CheckUTurnOnRoute(resultTest, 1 /* outgoingSegmentIndex */, NumMwmIds(), turn1), 1, ());
  TEST_EQUAL(turn1.m_turn, CarDirection::UTurnLeft, ());
  TurnItem turn2;
  TEST_EQUAL(CheckUTurnOnRoute(resultTest, 2 /* outgoingSegmentIndex */, NumMwmIds(), turn2), 1, ());
  TEST_EQUAL(turn2.m_turn, CarDirection::UTurnLeft, ());

  // Empty path test.
  TurnItem turn3;
  TEST_EQUAL(CheckUTurnOnRoute(resultTest, 3 /* outgoingSegmentIndex */, NumMwmIds(), turn3), 0, ());
}

UNIT_TEST(GetNextRoutePointIndex)
{
  TUnpackedPathSegments pathSegments(2, LoadedPathSegment());
  pathSegments[0].m_path = {{{0, 0}, 0}, {{0, 1}, 0}, {{0, 2}, 0}};
  pathSegments[1].m_path = {{{0, 2}, 0}, {{1, 2}, 0}};

  RoutingResultTest resultTest(pathSegments);
  RoutePointIndex nextIndex;

  // Forward direction.
  TEST(GetNextRoutePointIndex(resultTest,
                              RoutePointIndex({0 /* m_segmentIndex */, 0 /* m_pathIndex */}),
                              NumMwmIds(), true /* forward */, nextIndex), ());
  TEST_EQUAL(nextIndex, RoutePointIndex({0 /* m_segmentIndex */, 1 /* m_pathIndex */}), ());

  TEST(GetNextRoutePointIndex(resultTest,
                              RoutePointIndex({0 /* m_segmentIndex */, 1 /* m_pathIndex */}),
                              NumMwmIds(), true /* forward */, nextIndex), ());
  TEST_EQUAL(nextIndex, RoutePointIndex({0 /* m_segmentIndex */, 2 /* m_pathIndex */}), ());

  // Trying to get next item after the last item of the first segment.
  TEST(!GetNextRoutePointIndex(resultTest,
                               RoutePointIndex({0 /* m_segmentIndex */, 2 /* m_pathIndex */}),
                               NumMwmIds(), true /* forward */, nextIndex), ());

  // Trying to get point about the end of the route.
  TEST(!GetNextRoutePointIndex(resultTest,
                               RoutePointIndex({1 /* m_segmentIndex */, 1 /* m_pathIndex */}),
                               NumMwmIds(), true /* forward */, nextIndex), ());

  // Backward direction.
  // Moving in backward direction it's possible to get index of the first item of a segment.
  TEST(GetNextRoutePointIndex(resultTest,
                              RoutePointIndex({1 /* m_segmentIndex */, 1 /* m_pathIndex */}),
                              NumMwmIds(), false /* forward */, nextIndex), ());
  TEST_EQUAL(nextIndex, RoutePointIndex({1 /* m_segmentIndex */, 0 /* m_pathIndex */}), ());

  TEST(GetNextRoutePointIndex(resultTest,
                              RoutePointIndex({0 /* m_segmentIndex */, 2 /* m_pathIndex */}),
                              NumMwmIds(), false /* forward */, nextIndex), ());
  TEST_EQUAL(nextIndex, RoutePointIndex({0 /* m_segmentIndex */, 1 /* m_pathIndex */}), ());

  TEST(GetNextRoutePointIndex(resultTest,
                              RoutePointIndex({0 /* m_segmentIndex */, 1 /* m_pathIndex */}),
                              NumMwmIds(), false /* forward */, nextIndex), ());
  TEST_EQUAL(nextIndex, RoutePointIndex({0 /* m_segmentIndex */, 0 /* m_pathIndex */}), ());

  // Trying to get point before the beginning.
  TEST(!GetNextRoutePointIndex(resultTest,
                               RoutePointIndex({0 /* m_segmentIndex */, 0 /* m_pathIndex */}),
                               NumMwmIds(), false /* forward */, nextIndex), ());
}
}  // namespace
