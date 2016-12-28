#include "testing/testing.hpp"

#include "routing/route.hpp"
#include "routing/turns.hpp"
#include "routing/turns_generator.hpp"

#include "indexer/ftypes_matcher.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "std/cmath.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

using namespace routing;
using namespace turns;

namespace
{
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
  m2::RectD const kSquareNearZero = MercatorBounds::MetresToXY(kSquareCenterLonLat.x,
                                                               kSquareCenterLonLat.y, kHalfSquareSideMeters);
  // Removing a turn in case staying on a roundabout.
  vector<Junction> const pointsMerc1 = {
    {{ kSquareNearZero.minX(), kSquareNearZero.minY() }, feature::kDefaultAltitudeMeters},
    {{ kSquareNearZero.minX(), kSquareNearZero.maxY() }, feature::kDefaultAltitudeMeters},
    {{ kSquareNearZero.maxX(), kSquareNearZero.maxY() }, feature::kDefaultAltitudeMeters},
    {{ kSquareNearZero.maxX(), kSquareNearZero.minY() }, feature::kDefaultAltitudeMeters},
  };
  // The constructor TurnItem(uint32_t idx, TurnDirection t, uint32_t exitNum = 0)
  // is used for initialization of vector<TurnItem> below.
  Route::TTurns turnsDir1 = {{0, TurnDirection::EnterRoundAbout},
                             {1, TurnDirection::StayOnRoundAbout},
                             {2, TurnDirection::LeaveRoundAbout},
                             {3, TurnDirection::ReachedYourDestination}};

  FixupTurns(pointsMerc1, turnsDir1);
  Route::TTurns const expectedTurnDir1 = {{0, TurnDirection::EnterRoundAbout, 2},
                                          {2, TurnDirection::LeaveRoundAbout, 2},
                                          {3, TurnDirection::ReachedYourDestination}};
  TEST_EQUAL(turnsDir1, expectedTurnDir1, ());

  // Merging turns which are close to each other.
  vector<Junction> const pointsMerc2 = {
    {{ kSquareNearZero.minX(), kSquareNearZero.minY()}, feature::kDefaultAltitudeMeters},
    {{ kSquareCenterLonLat.x, kSquareCenterLonLat.y }, feature::kDefaultAltitudeMeters},
    {{ kSquareNearZero.maxX(), kSquareNearZero.maxY() }, feature::kDefaultAltitudeMeters},
  };
  Route::TTurns turnsDir2 = {{0, TurnDirection::GoStraight},
                             {1, TurnDirection::TurnLeft},
                             {2, TurnDirection::ReachedYourDestination}};

  FixupTurns(pointsMerc2, turnsDir2);
  Route::TTurns const expectedTurnDir2 = {{1, TurnDirection::TurnLeft},
                                          {2, TurnDirection::ReachedYourDestination}};
  TEST_EQUAL(turnsDir2, expectedTurnDir2, ());

  // No turn is removed.
  vector<Junction> const pointsMerc3 = {
    {{ kSquareNearZero.minX(), kSquareNearZero.minY()}, feature::kDefaultAltitudeMeters},
    {{ kSquareNearZero.minX(), kSquareNearZero.maxY() }, feature::kDefaultAltitudeMeters},
    {{ kSquareNearZero.maxX(), kSquareNearZero.maxY() }, feature::kDefaultAltitudeMeters},
  };
  Route::TTurns turnsDir3 = {{1, TurnDirection::TurnRight},
                             {2, TurnDirection::ReachedYourDestination}};

  FixupTurns(pointsMerc3, turnsDir3);
  Route::TTurns const expectedTurnDir3 = {{1, TurnDirection::TurnRight},
                                          {2, TurnDirection::ReachedYourDestination}};
  TEST_EQUAL(turnsDir3, expectedTurnDir3, ());
}

UNIT_TEST(TestIsLaneWayConformedTurnDirection)
{
  TEST(IsLaneWayConformedTurnDirection(LaneWay::Left, TurnDirection::TurnLeft), ());
  TEST(IsLaneWayConformedTurnDirection(LaneWay::Right, TurnDirection::TurnRight), ());
  TEST(IsLaneWayConformedTurnDirection(LaneWay::SlightLeft, TurnDirection::TurnSlightLeft), ());
  TEST(IsLaneWayConformedTurnDirection(LaneWay::SharpRight, TurnDirection::TurnSharpRight), ());
  TEST(IsLaneWayConformedTurnDirection(LaneWay::Reverse, TurnDirection::UTurnLeft), ());
  TEST(IsLaneWayConformedTurnDirection(LaneWay::Reverse, TurnDirection::UTurnRight), ());
  TEST(IsLaneWayConformedTurnDirection(LaneWay::Through, TurnDirection::GoStraight), ());

  TEST(!IsLaneWayConformedTurnDirection(LaneWay::Left, TurnDirection::TurnSlightLeft), ());
  TEST(!IsLaneWayConformedTurnDirection(LaneWay::Right, TurnDirection::TurnSharpRight), ());
  TEST(!IsLaneWayConformedTurnDirection(LaneWay::SlightLeft, TurnDirection::GoStraight), ());
  TEST(!IsLaneWayConformedTurnDirection(LaneWay::SharpRight, TurnDirection::NoTurn), ());
  TEST(!IsLaneWayConformedTurnDirection(LaneWay::Reverse, TurnDirection::TurnLeft), ());
  TEST(!IsLaneWayConformedTurnDirection(LaneWay::None, TurnDirection::ReachedYourDestination), ());
}

UNIT_TEST(TestIsLaneWayConformedTurnDirectionApproximately)
{
  TEST(IsLaneWayConformedTurnDirectionApproximately(LaneWay::Left, TurnDirection::TurnSharpLeft), ());
  TEST(IsLaneWayConformedTurnDirectionApproximately(LaneWay::Left, TurnDirection::TurnSlightLeft), ());
  TEST(IsLaneWayConformedTurnDirectionApproximately(LaneWay::Right, TurnDirection::TurnSharpRight), ());
  TEST(IsLaneWayConformedTurnDirectionApproximately(LaneWay::Right, TurnDirection::TurnRight), ());
  TEST(IsLaneWayConformedTurnDirectionApproximately(LaneWay::Reverse, TurnDirection::UTurnLeft), ());
  TEST(IsLaneWayConformedTurnDirectionApproximately(LaneWay::Reverse, TurnDirection::UTurnRight), ());
  TEST(IsLaneWayConformedTurnDirectionApproximately(LaneWay::SlightLeft, TurnDirection::GoStraight), ());
  TEST(IsLaneWayConformedTurnDirectionApproximately(LaneWay::SlightRight, TurnDirection::GoStraight), ());

  TEST(!IsLaneWayConformedTurnDirectionApproximately(LaneWay::SharpLeft, TurnDirection::UTurnLeft), ());
  TEST(!IsLaneWayConformedTurnDirectionApproximately(LaneWay::SharpLeft, TurnDirection::UTurnRight), ());
  TEST(!IsLaneWayConformedTurnDirectionApproximately(LaneWay::SharpRight, TurnDirection::UTurnLeft), ());
  TEST(!IsLaneWayConformedTurnDirectionApproximately(LaneWay::SharpRight, TurnDirection::UTurnRight), ());
  TEST(!IsLaneWayConformedTurnDirection(LaneWay::Through, TurnDirection::ReachedYourDestination), ());
  TEST(!IsLaneWayConformedTurnDirectionApproximately(LaneWay::Through, TurnDirection::TurnRight), ());
  TEST(!IsLaneWayConformedTurnDirectionApproximately(LaneWay::SlightRight,
                                                     TurnDirection::TurnSharpLeft), ());
}

UNIT_TEST(TestAddingActiveLaneInformation)
{
  Route::TTurns turns = {{0, TurnDirection::GoStraight},
                         {1, TurnDirection::TurnLeft},
                         {2, TurnDirection::ReachedYourDestination}};
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
  TEST_EQUAL(GetRoundaboutDirection(true, true, true, true), TurnDirection::StayOnRoundAbout, ());
  TEST_EQUAL(GetRoundaboutDirection(true, true, true, false), TurnDirection::NoTurn, ());
  TEST_EQUAL(GetRoundaboutDirection(true, true, false, true), TurnDirection::NoTurn, ());
  TEST_EQUAL(GetRoundaboutDirection(true, true, false, false), TurnDirection::NoTurn, ());
  TEST_EQUAL(GetRoundaboutDirection(false, true, false, true), TurnDirection::EnterRoundAbout, ());
  TEST_EQUAL(GetRoundaboutDirection(true, false, false, false), TurnDirection::LeaveRoundAbout, ());
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
  TEST_EQUAL(InvertDirection(TurnDirection::TurnSlightRight), TurnDirection::TurnSlightLeft, ());
  TEST_EQUAL(InvertDirection(TurnDirection::TurnRight), TurnDirection::TurnLeft, ());
  TEST_EQUAL(InvertDirection(TurnDirection::TurnSharpRight), TurnDirection::TurnSharpLeft, ());
  TEST_EQUAL(InvertDirection(TurnDirection::TurnSlightLeft), TurnDirection::TurnSlightRight, ());
  TEST_EQUAL(InvertDirection(TurnDirection::TurnSlightRight), TurnDirection::TurnSlightLeft, ());
  TEST_EQUAL(InvertDirection(TurnDirection::TurnLeft), TurnDirection::TurnRight, ());
  TEST_EQUAL(InvertDirection(TurnDirection::TurnSharpLeft), TurnDirection::TurnSharpRight, ());
}

UNIT_TEST(TestRightmostDirection)
{
  TEST_EQUAL(RightmostDirection(180.), TurnDirection::TurnSharpRight, ());
  TEST_EQUAL(RightmostDirection(170.), TurnDirection::TurnSharpRight, ());
  TEST_EQUAL(RightmostDirection(90.), TurnDirection::TurnRight, ());
  TEST_EQUAL(RightmostDirection(45.), TurnDirection::TurnRight, ());
  TEST_EQUAL(RightmostDirection(0.), TurnDirection::TurnSlightRight, ());
  TEST_EQUAL(RightmostDirection(-20.), TurnDirection::GoStraight, ());
  TEST_EQUAL(RightmostDirection(-90.), TurnDirection::TurnLeft, ());
  TEST_EQUAL(RightmostDirection(-170.), TurnDirection::TurnSharpLeft, ());
}

UNIT_TEST(TestLeftmostDirection)
{
  TEST_EQUAL(LeftmostDirection(180.), TurnDirection::TurnSharpRight, ());
  TEST_EQUAL(LeftmostDirection(170.), TurnDirection::TurnSharpRight, ());
  TEST_EQUAL(LeftmostDirection(90.), TurnDirection::TurnRight, ());
  TEST_EQUAL(LeftmostDirection(45.), TurnDirection::TurnSlightRight, ());
  TEST_EQUAL(LeftmostDirection(0.), TurnDirection::TurnSlightLeft, ());
  TEST_EQUAL(LeftmostDirection(-20.), TurnDirection::TurnSlightLeft, ());
  TEST_EQUAL(LeftmostDirection(-90.), TurnDirection::TurnLeft, ());
  TEST_EQUAL(LeftmostDirection(-170.), TurnDirection::TurnSharpLeft, ());
}

UNIT_TEST(TestIntermediateDirection)
{
  TEST_EQUAL(IntermediateDirection(180.), TurnDirection::TurnSharpRight, ());
  TEST_EQUAL(IntermediateDirection(170.), TurnDirection::TurnSharpRight, ());
  TEST_EQUAL(IntermediateDirection(90.), TurnDirection::TurnRight, ());
  TEST_EQUAL(IntermediateDirection(45.), TurnDirection::TurnSlightRight, ());
  TEST_EQUAL(IntermediateDirection(0.), TurnDirection::GoStraight, ());
  TEST_EQUAL(IntermediateDirection(-20.), TurnDirection::TurnSlightLeft, ());
  TEST_EQUAL(IntermediateDirection(-90.), TurnDirection::TurnLeft, ());
  TEST_EQUAL(IntermediateDirection(-170.), TurnDirection::TurnSharpLeft, ());
}

UNIT_TEST(TestCalculateMercatorDistanceAlongRoute)
{
  vector<m2::PointD> const points = {{0., 0.}, {0., 1.}, {0., 1.}, {1., 1.}};

  TEST_EQUAL(CalculateMercatorDistanceAlongPath(0, points.size() - 1, points), 2., ());
  TEST_EQUAL(CalculateMercatorDistanceAlongPath(1, 1, points), 0., ());
  TEST_EQUAL(CalculateMercatorDistanceAlongPath(1, 2, points), 0., ());
  TEST_EQUAL(CalculateMercatorDistanceAlongPath(0, 1, points), 1., ());
}

UNIT_TEST(TestCheckUTurnOnRoute)
{
  TUnpackedPathSegments pathSegments(4, LoadedPathSegment(UniNodeId::Type::Osrm));
  pathSegments[0].m_name = "A road";
  pathSegments[0].m_weight = 1;
  pathSegments[0].m_nodeId = UniNodeId(0 /* node id */);
  pathSegments[0].m_highwayClass = ftypes::HighwayClass::Trunk;
  pathSegments[0].m_onRoundabout = false;
  pathSegments[0].m_isLink = false;
  pathSegments[0].m_path = {{{0, 0}, 0}, {{0, 1}, 0}};

  pathSegments[1] = pathSegments[0];
  pathSegments[1].m_nodeId = UniNodeId(1 /* node id */);
  pathSegments[1].m_path = {{{0, 1}, 0}, {{0, 0}, 0}};

  pathSegments[2] = pathSegments[0];
  pathSegments[2].m_nodeId = UniNodeId(2 /* node id */);
  pathSegments[2].m_path = {{{0, 0}, 0}, {{0, 1}, 0}};

  pathSegments[3] = pathSegments[0];
  pathSegments[3].m_nodeId = UniNodeId(3 /* node id */);
  pathSegments[3].m_path.clear();

  // Zigzag test.
  TurnItem turn1;
  TEST_EQUAL(CheckUTurnOnRoute(pathSegments, 1, turn1), 1, ());
  TEST_EQUAL(turn1.m_turn, TurnDirection::UTurnLeft, ());
  TurnItem turn2;
  TEST_EQUAL(CheckUTurnOnRoute(pathSegments, 2, turn2), 1, ());
  TEST_EQUAL(turn2.m_turn, TurnDirection::UTurnLeft, ());

  // Empty path test.
  TurnItem turn3;
  TEST_EQUAL(CheckUTurnOnRoute(pathSegments, 3, turn3), 0, ());
}
}  // namespace
