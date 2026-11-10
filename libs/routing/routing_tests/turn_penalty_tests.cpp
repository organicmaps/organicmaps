#include "testing/testing.hpp"

#include "generator/generator_tests_support/routing_helpers.hpp"

#include "traffic/traffic_cache.hpp"

#include "routing/routing_tests/index_graph_tools.hpp"

#include "routing/edge_estimator.hpp"
#include "routing/fake_edges_container.hpp"
#include "routing/geometry.hpp"
#include "routing/index_graph.hpp"
#include "routing/index_graph_starter_joints.hpp"
#include "routing/joint_segment.hpp"
#include "routing/routing_helpers.hpp"

#include "indexer/classificator_loader.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "base/math.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

namespace turn_penalty_tests
{
using namespace routing;
using namespace routing_test;
using namespace std;

using Algorithm = AStarAlgorithm<Segment, SegmentEdge, RouteWeight>;
using Purpose = EdgeEstimator::Purpose;

double constexpr kServiceTurnPenaltyS = 30.0;
double constexpr kWeightEps = 1e-6;

UNIT_TEST(TurnPenalty_CarEstimator)
{
  classificator::Load();
  traffic::TrafficCache const trafficCache;
  auto const estimator = CreateEstimatorForCar(trafficCache);
  auto const penalty = [&](HighwayType from, HighwayType to, Purpose purpose = Purpose::Weight)
  { return estimator->GetTurnPenalty(purpose, from, to); };

  TEST_EQUAL(penalty(HighwayType::HighwayResidential, HighwayType::HighwayService), kServiceTurnPenaltyS, ());
  TEST_EQUAL(penalty(HighwayType::HighwayService, HighwayType::HighwayPrimary), kServiceTurnPenaltyS, ());
  TEST_EQUAL(penalty(HighwayType::HighwayLivingStreet, HighwayType::HighwayTertiary), kServiceTurnPenaltyS, ());
  TEST_EQUAL(penalty(HighwayType::HighwayService, HighwayType::HighwayLivingStreet), 0.0, ());
  TEST_EQUAL(penalty(HighwayType::HighwayPrimary, HighwayType::HighwayResidential), 0.0, ());
  // A route preference: the ETA does not include it.
  TEST_EQUAL(penalty(HighwayType::HighwayResidential, HighwayType::HighwayService, Purpose::ETA), 0.0, ());
  TEST_EQUAL(estimator->GetTurnPenalty(Purpose::Weight, {}, HighwayType::HighwayService), 0.0, ());
}

//          N
//          |
//          F2 (service)
//          |
// W --F0-- J --F1-- E      F0, F3 residential, F1 service
//          |
//          F3
//          |
//          S
unique_ptr<SingleVehicleWorldGraph> BuildJunctionGraph(shared_ptr<EdgeEstimator> estimator = nullptr)
{
  auto loader = make_unique<TestGeometryLoader>();
  vector<RoadGeometry::Points> const roads = {
      {{-0.001, 0}, {0, 0}}, {{0, 0}, {0.001, 0}}, {{0, 0}, {0, 0.001}}, {{0, 0}, {0, -0.001}}};
  vector<HighwayType> const types = {HighwayType::HighwayResidential, HighwayType::HighwayService,
                                     HighwayType::HighwayService, HighwayType::HighwayResidential};
  for (uint32_t featureId = 0; featureId < roads.size(); ++featureId)
  {
    loader->AddRoad(featureId, false /* oneWay */, 50.0 /* speed */, roads[featureId]);
    loader->SetHighwayType(featureId, types[featureId]);
  }

  vector<Joint> const joints = {MakeJoint({{0, 0}}), MakeJoint({{0, 1}, {1, 0}, {2, 0}, {3, 0}}), MakeJoint({{1, 1}}),
                                MakeJoint({{2, 1}}), MakeJoint({{3, 1}})};

  traffic::TrafficCache const trafficCache;
  return BuildWorldGraph(std::move(loader), estimator ? estimator : CreateEstimatorForCar(trafficCache), joints);
}

double GetEdgeWeight(IndexGraph const & graph, Segment const & from, Segment const & to, bool isOutgoing)
{
  IndexGraph::SegmentEdgeListT edges;
  graph.GetEdgeList(from, isOutgoing, true /* useRoutingOptions */, edges);
  auto const it = find_if(edges.begin(), edges.end(), [&to](SegmentEdge const & e) { return e.GetTarget() == to; });
  TEST(it != edges.end(), (from, to, isOutgoing));
  return it->GetWeight().GetWeight();
}

UNIT_TEST(TurnPenalty_EdgeWeights)
{
  classificator::Load();
  auto worldGraph = BuildJunctionGraph();
  auto const & graph = worldGraph->GetIndexGraphForTests(kTestNumMwmId);

  Segment const fromW(kTestNumMwmId, 0, 0, true /* forward */);
  Segment const toE(kTestNumMwmId, 1, 0, true /* forward */);
  Segment const toN(kTestNumMwmId, 2, 0, true /* forward */);
  Segment const toS(kTestNumMwmId, 3, 0, true /* forward */);
  Segment const backToW(kTestNumMwmId, 0, 0, false /* forward */);

  // All the segments have the same length and speed, so the weights differ by the penalties only.
  double const straight = GetEdgeWeight(graph, fromW, toE, true /* isOutgoing */);
  TEST_ALMOST_EQUAL_ABS(GetEdgeWeight(graph, fromW, toN, true /* isOutgoing */), straight + kServiceTurnPenaltyS, 1e-6,
                        ("A turn onto a service road"));
  TEST_ALMOST_EQUAL_ABS(GetEdgeWeight(graph, fromW, toS, true /* isOutgoing */), straight, 1e-6,
                        ("A turn without a road type change"));

  // The backward wave gets the same weights.
  TEST_ALMOST_EQUAL_ABS(GetEdgeWeight(graph, toN, fromW, false /* isOutgoing */), straight + kServiceTurnPenaltyS, 1e-6,
                        ());
  TEST_ALMOST_EQUAL_ABS(GetEdgeWeight(graph, toE, fromW, false /* isOutgoing */), straight, 1e-6, ());

  // A U-turn on the same road has the U-turn penalty only.
  traffic::TrafficCache const trafficCache;
  double const uTurnPenalty = CreateEstimatorForCar(trafficCache)->GetUTurnPenalty(Purpose::Weight);
  TEST_ALMOST_EQUAL_ABS(GetEdgeWeight(graph, fromW, backToW, true /* isOutgoing */), straight + uTurnPenalty, 1e-6, ());
}

// A route leaves the service road of its start and enters the one of its finish for free,
// the opposite turns pay for both turns of a passage through a service road.
UNIT_TEST(TurnPenalty_ServiceEndings)
{
  classificator::Load();
  traffic::TrafficCache const trafficCache;
  auto const estimator = CreateEstimatorForCar(trafficCache);
  auto worldGraph = BuildJunctionGraph(estimator);
  auto const & graph = worldGraph->GetIndexGraphForTests(kTestNumMwmId);

  Segment const fromW(kTestNumMwmId, 0, 0, true /* forward */);
  Segment const toW(kTestNumMwmId, 0, 0, false /* forward */);
  Segment const fromN(kTestNumMwmId, 2, 0, false /* forward */);
  Segment const toN(kTestNumMwmId, 2, 0, true /* forward */);
  Segment const toS(kTestNumMwmId, 3, 0, true /* forward */);
  // All the segments have the same weight.
  double const noPenalty = GetEdgeWeight(graph, fromW, toS, true /* isOutgoing */);
  auto const testPenalties = [&](double onto, double off)
  {
    TEST_ALMOST_EQUAL_ABS(GetEdgeWeight(graph, fromW, toN, true /* isOutgoing */), noPenalty + onto, kWeightEps, ());
    TEST_ALMOST_EQUAL_ABS(GetEdgeWeight(graph, toN, fromW, false /* isOutgoing */), noPenalty + onto, kWeightEps, ());
    TEST_ALMOST_EQUAL_ABS(GetEdgeWeight(graph, fromN, toW, true /* isOutgoing */), noPenalty + off, kWeightEps, ());
  };

  auto const west = mercator::ToLatLon(m2::PointD(-0.001, 0));
  auto const east = mercator::ToLatLon(m2::PointD(0.002, 0));
  testPenalties(kServiceTurnPenaltyS, kServiceTurnPenaltyS);

  estimator->SetServiceEndings({west /* start */, {} /* finish */});
  testPenalties(2 * kServiceTurnPenaltyS, 0.0);

  estimator->SetServiceEndings({{} /* start */, west /* finish */});
  testPenalties(0.0, 2 * kServiceTurnPenaltyS);

  // The junction is closer to the west ending.
  estimator->SetServiceEndings({west /* start */, east /* finish */});
  testPenalties(2 * kServiceTurnPenaltyS, 0.0);
  estimator->SetServiceEndings({east /* start */, west /* finish */});
  testPenalties(0.0, 2 * kServiceTurnPenaltyS);
}

// An edge to a part of a segment has the penalties of the edge to the whole segment.
UNIT_CLASS_TEST(RestrictionTest, TurnPenalty_PartOfSegment)
{
  Init(BuildJunctionGraph());
  // The finish is in the middle of the service road F2.
  SetStarter(MakeFakeEnding(0, 0, m2::PointD(-0.001, 0), *m_graph),
             MakeFakeEnding(2, 0, m2::PointD(0, 0.0005), *m_graph));

  Segment const fromW(kTestNumMwmId, 0, 0, true /* forward */);
  IndexGraphStarter::EdgeListT edges;
  m_starter->GetEdgesList(fromW, true /* isOutgoing */, edges);
  auto it = find_if(edges.begin(), edges.end(),
                    [](SegmentEdge const & e) { return IndexGraphStarter::IsFakeSegment(e.GetTarget()); });
  TEST(it != edges.end(), ());
  Segment const part = it->GetTarget();
  double const expected = m_starter->CalcSegmentWeight(part, Purpose::Weight).GetWeight() + kServiceTurnPenaltyS;
  TEST_ALMOST_EQUAL_ABS(it->GetWeight().GetWeight(), expected, kWeightEps, ());

  // The backward wave gets the same weight.
  m_starter->GetEdgesList(part, false /* isOutgoing */, edges);
  it = find_if(edges.begin(), edges.end(), [&fromW](SegmentEdge const & e) { return e.GetTarget() == fromW; });
  TEST(it != edges.end(), ());
  TEST_ALMOST_EQUAL_ABS(it->GetWeight().GetWeight(), expected, kWeightEps, ());
}

// Returns the penalty between the |residential| road and the |service| one, which starts at its last point.
// |reverse| goes from the service road to the residential one.
double GetTypeChangePenalty(RoadGeometry::Points const & residential, RoadGeometry::Points const & service,
                            bool reverse)
{
  auto const lastPointId = static_cast<uint32_t>(residential.size() - 1);
  auto const getWeight = [&](HighwayType serviceRoadType)
  {
    auto loader = make_unique<TestGeometryLoader>();
    loader->AddRoad(0, false /* oneWay */, 50.0 /* speed */, residential);
    loader->SetHighwayType(0, HighwayType::HighwayResidential);
    loader->AddRoad(1, false /* oneWay */, 50.0 /* speed */, service);
    loader->SetHighwayType(1, serviceRoadType);

    traffic::TrafficCache const trafficCache;
    auto const worldGraph = BuildWorldGraph(std::move(loader), CreateEstimatorForCar(trafficCache),
                                            {MakeJoint({{0, lastPointId}, {1, 0}})});
    Segment const residentialSegment(kTestNumMwmId, 0, lastPointId - 1, !reverse /* forward */);
    Segment const serviceSegment(kTestNumMwmId, 1, 0, !reverse /* forward */);
    return GetEdgeWeight(worldGraph->GetIndexGraphForTests(kTestNumMwmId),
                         reverse ? serviceSegment : residentialSegment, reverse ? residentialSegment : serviceSegment,
                         true /* isOutgoing */);
  };
  // The same roads have no penalty without the road type change.
  return getWeight(HighwayType::HighwayService) - getWeight(HighwayType::HighwayResidential);
}

UNIT_TEST(TurnPenalty_Angle)
{
  classificator::Load();
  for (bool const reverse : {false, true})
  {
    auto const penalty = [reverse](double angleDeg)
    {
      double const angle = math::DegToRad(angleDeg);
      return GetTypeChangePenalty({{-0.001, 0}, {0, 0}}, {{0, 0}, {0.001 * cos(angle), 0.001 * sin(angle)}}, reverse);
    };

    // A direction change of 30 degrees or more is a turn.
    for (double const angleDeg : {0.0, 29.0, -29.0})
      TEST_ALMOST_EQUAL_ABS(penalty(angleDeg), 0.0, kWeightEps, (angleDeg, reverse));
    for (double const angleDeg : {31.0, -31.0, 90.0, 150.0})
      TEST_ALMOST_EQUAL_ABS(penalty(angleDeg), kServiceTurnPenaltyS, kWeightEps, (angleDeg, reverse));
  }
}

UNIT_TEST(TurnPenalty_ZeroLengthSegments)
{
  classificator::Load();
  for (bool const reverse : {false, true})
  {
    // The roads meet at (0, 0) with zero length segments, the turn is seen by the next points.
    TEST_ALMOST_EQUAL_ABS(GetTypeChangePenalty({{-0.001, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0.001}}, reverse),
                          kServiceTurnPenaltyS, kWeightEps, (reverse));
    TEST_ALMOST_EQUAL_ABS(GetTypeChangePenalty({{-0.001, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0.001}}, reverse),
                          kServiceTurnPenaltyS, kWeightEps, (reverse));
    TEST_ALMOST_EQUAL_ABS(GetTypeChangePenalty({{-0.001, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, -0.001}}, reverse),
                          kServiceTurnPenaltyS, kWeightEps, (reverse));
    TEST_ALMOST_EQUAL_ABS(GetTypeChangePenalty({{-0.001, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0.001, 0}}, reverse),
                          0.0, kWeightEps, (reverse));
  }
}

// S --F6-- * --F0-- A ------F3------ E
//                   |              /
//                   +----F1----- C  F4      F1 service, the others residential
//                                |
//                                F2
//                                |
//                                * --F5-- F
// The detour F3, F4 is 69 m (5 s) longer than the service road F1 with two turns.
unique_ptr<SingleVehicleWorldGraph> BuildServiceShortcutGraph(double detourSpeedKMpH = 50.0)
{
  auto loader = make_unique<TestGeometryLoader>();
  vector<RoadGeometry::Points> const roads = {
      {{0, 0}, {0.001, 0}},                          // F0
      {{0.001, 0}, {0.001, 0.001}, {0.002, 0.001}},  // F1
      {{0.002, 0.001}, {0.002, 0.002}},              // F2
      {{0.001, 0}, {0.0025, 0}},                     // F3
      {{0.0025, 0}, {0.002, 0.001}},                 // F4
      {{0.002, 0.002}, {0.002, 0.003}},              // F5
      {{-0.001, 0}, {0, 0}},                         // F6
  };
  for (uint32_t featureId = 0; featureId < roads.size(); ++featureId)
  {
    bool const isDetour = featureId == 3 || featureId == 4;
    loader->AddRoad(featureId, false /* oneWay */, isDetour ? detourSpeedKMpH : 50.0, roads[featureId]);
    loader->SetHighwayType(featureId, featureId == 1 ? HighwayType::HighwayService : HighwayType::HighwayResidential);
  }

  vector<Joint> const joints = {
      MakeJoint({{6, 0}}),         MakeJoint({{6, 1}, {0, 0}}),         MakeJoint({{0, 1}, {1, 0}, {3, 0}}),
      MakeJoint({{3, 1}, {4, 0}}), MakeJoint({{1, 2}, {4, 1}, {2, 0}}), MakeJoint({{2, 1}, {5, 0}}),
      MakeJoint({{5, 1}}),
  };

  traffic::TrafficCache const trafficCache;
  return BuildWorldGraph(std::move(loader), CreateEstimatorForCar(trafficCache), joints);
}

// Calculates the route by joints, as the car routing does, and returns its weight.
double CalculateJointsRoute(IndexGraphStarter & starter, vector<Segment> & route)
{
  using JointsStarter = IndexGraphStarterJoints<IndexGraphStarter>;
  using JointsAlgorithm = AStarAlgorithm<JointSegment, JointEdge, RouteWeight>;

  // The test graph has no cross-mwm section.
  auto & graph = starter.GetGraph();
  auto const mode = graph.GetMode();
  graph.SetMode(WorldGraphMode::JointSingleMwm);

  JointsStarter jointsStarter(starter, starter.GetStartSegment(), starter.GetFinishSegment());
  JointsAlgorithm::ParamsForTests<AStarLengthChecker> params(
      jointsStarter, jointsStarter.GetStartJoint(), jointsStarter.GetFinishJoint(), AStarLengthChecker(starter));
  RoutingResult<JointSegment, RouteWeight> result;
  TEST_EQUAL(JointsAlgorithm().FindPathBidirectional(params, result), JointsAlgorithm::Result::OK, ());

  route.clear();
  for (auto const & joint : result.m_path)
    for (auto const & segment : jointsStarter.ReconstructJoint(joint))
      if (route.empty() || route.back() != segment)
        route.push_back(segment);

  graph.SetMode(mode);
  return result.m_distance.GetWeight();
}

void TestGeometry(IndexGraphStarter const & starter, vector<Segment> const & route,
                  vector<m2::PointD> const & expectedGeom)
{
  vector<m2::PointD> geom;
  for (auto const & segment : route)
  {
    for (bool const front : {false, true})
    {
      auto const point = mercator::FromLatLon(starter.GetPoint(segment, front));
      if (geom.empty() || !AlmostEqualAbs(geom.back(), point, 1e-8))
        geom.push_back(point);
    }
  }

  TEST_EQUAL(geom.size(), expectedGeom.size(), (geom, expectedGeom));
  for (size_t i = 0; i < geom.size(); ++i)
    TEST(AlmostEqualAbs(geom[i], expectedGeom[i], 1e-8), (geom, expectedGeom));
}

// Checks the geometry and the weight of the routes calculated by segments and by joints.
void TestRoute(IndexGraphStarter & starter, vector<m2::PointD> const & expectedGeom, double expectedPenalty)
{
  for (bool const joints : {false, true})
  {
    vector<Segment> route;
    double weight = 0.0;
    if (joints)
      weight = CalculateJointsRoute(starter, route);
    else
      TEST_EQUAL(CalculateRoute(starter, route, weight), Algorithm::Result::OK, ());

    TestGeometry(starter, route, expectedGeom);

    // The route weight is the sum of its edges: the segments with the transition penalties.
    double segmentsWeight = 0.0;
    double edgesWeight = 0.0;
    for (size_t i = 0; i < route.size(); ++i)
    {
      segmentsWeight += starter.CalcSegmentWeight(route[i], Purpose::Weight).GetWeight();
      if (i > 0)
        edgesWeight += starter.CalcRouteEdgeWeight(route[i - 1], route[i]).GetWeight();
    }
    TEST_ALMOST_EQUAL_ABS(weight, segmentsWeight + expectedPenalty, kWeightEps, (joints));
    TEST_ALMOST_EQUAL_ABS(weight, edgesWeight, kWeightEps, (joints));
  }
}

UNIT_CLASS_TEST(RestrictionTest, TurnPenalty_AvoidServiceShortcut)
{
  for (bool const reverse : {false, true})
  {
    Init(BuildServiceShortcutGraph());
    auto const start = MakeFakeEnding(6, 0, m2::PointD(-0.001, 0), *m_graph);
    auto const finish = MakeFakeEnding(5, 0, m2::PointD(0.002, 0.003), *m_graph);
    SetStarter(reverse ? finish : start, reverse ? start : finish);

    vector<m2::PointD> expectedGeom = {{-0.001, 0},    {0, 0},         {0.001, 0},    {0.0025, 0},
                                       {0.002, 0.001}, {0.002, 0.002}, {0.002, 0.003}};
    if (reverse)
      std::reverse(expectedGeom.begin(), expectedGeom.end());
    TestRoute(*m_starter, expectedGeom, 0.0 /* expectedPenalty */);

    // The service road is used when there is no other way.
    SetRestrictions({{0, 3}, {2, 4}});
    expectedGeom = {{-0.001, 0}, {0, 0}, {0.001, 0}, {0.001, 0.001}, {0.002, 0.001}, {0.002, 0.002}, {0.002, 0.003}};
    if (reverse)
      std::reverse(expectedGeom.begin(), expectedGeom.end());
    TestRoute(*m_starter, expectedGeom, 2 * kServiceTurnPenaltyS);
  }
}

//          B --F2-- * --F5-- D
//          |                 |
//          F1                F4      F1 residential, the others service
//          |                 |
// * --F0-- A -------F3------ C
// The route from F0 to F2 by the service roads F3, F4, F5 is 222 m (16 s) longer than by the street F1.
UNIT_CLASS_TEST(RestrictionTest, TurnPenalty_ServiceEndingsRoute)
{
  vector<RoadGeometry::Points> const roads = {
      {{0, 0}, {0.001, 0}},               // F0
      {{0.001, 0}, {0.001, 0.003}},       // F1
      {{0.001, 0.003}, {0.002, 0.003}},   // F2
      {{0.001, 0}, {0.0025, 0}},          // F3
      {{0.0025, 0}, {0.0025, 0.003}},     // F4
      {{0.0025, 0.003}, {0.002, 0.003}},  // F5
  };
  vector<Joint> const joints = {
      MakeJoint({{0, 0}}),         MakeJoint({{0, 1}, {1, 0}, {3, 0}}), MakeJoint({{1, 1}, {2, 0}}),
      MakeJoint({{3, 1}, {4, 0}}), MakeJoint({{4, 1}, {5, 0}}),         MakeJoint({{5, 1}, {2, 1}}),
  };
  m2::PointD const startPoint(0.0005, 0);
  m2::PointD const finishPoint(0.0015, 0.003);
  vector<m2::PointD> const streetGeom = {startPoint, {0.001, 0}, {0.001, 0.003}, finishPoint};

  traffic::TrafficCache const trafficCache;
  for (bool const reverse : {false, true})
  {
    auto loader = make_unique<TestGeometryLoader>();
    for (uint32_t featureId = 0; featureId < roads.size(); ++featureId)
    {
      loader->AddRoad(featureId, false /* oneWay */, 50.0 /* speed */, roads[featureId]);
      loader->SetHighwayType(featureId, featureId == 1 ? HighwayType::HighwayResidential : HighwayType::HighwayService);
    }
    auto const estimator = CreateEstimatorForCar(trafficCache);
    Init(BuildWorldGraph(std::move(loader), estimator, joints));
    auto const start = MakeFakeEnding(0, 0, startPoint, *m_graph);
    auto const finish = MakeFakeEnding(2, 0, finishPoint, *m_graph);
    SetStarter(reverse ? finish : start, reverse ? start : finish);

    // The turns onto and off the street make it slower than the service roads.
    vector<m2::PointD> expectedGeom = {startPoint,      {0.001, 0},     {0.0025, 0},
                                       {0.0025, 0.003}, {0.002, 0.003}, finishPoint};
    if (reverse)
      std::reverse(expectedGeom.begin(), expectedGeom.end());
    TestRoute(*m_starter, expectedGeom, 0.0 /* expectedPenalty */);

    // These turns are free for the endings on the service roads.
    auto const startLatLon = mercator::ToLatLon(reverse ? finishPoint : startPoint);
    auto const finishLatLon = mercator::ToLatLon(reverse ? startPoint : finishPoint);
    estimator->SetServiceEndings({startLatLon, finishLatLon});
    expectedGeom = streetGeom;
    if (reverse)
      std::reverse(expectedGeom.begin(), expectedGeom.end());
    TestRoute(*m_starter, expectedGeom, 0.0 /* expectedPenalty */);
  }
}

// The route starts or finishes in the middle of the service road F1.
UNIT_CLASS_TEST(RestrictionTest, TurnPenalty_PartialEnding)
{
  for (bool const reverse : {false, true})
  {
    Init(BuildServiceShortcutGraph());
    auto const start = MakeFakeEnding(6, 0, m2::PointD(-0.001, 0), *m_graph);
    auto const finish = MakeFakeEnding(1, 0, m2::PointD(0.001, 0.0005), *m_graph);
    SetStarter(reverse ? finish : start, reverse ? start : finish);

    vector<m2::PointD> expectedGeom = {{-0.001, 0}, {0, 0}, {0.001, 0}, {0.001, 0.0005}};
    if (reverse)
      std::reverse(expectedGeom.begin(), expectedGeom.end());
    TestRoute(*m_starter, expectedGeom, kServiceTurnPenaltyS);
  }
}

// Rerouting, like IndexRouter::AdjustRoute(): the previous route suffix keeps the turn penalties.
UNIT_CLASS_TEST(RestrictionTest, TurnPenalty_AdjustRoute)
{
  // The detour is slow, so the previous route takes the service road.
  Init(BuildServiceShortcutGraph(12.0 /* detourSpeedKMpH */));
  auto const finish = MakeFakeEnding(5, 0, m2::PointD(0.002, 0.003), *m_graph);
  SetStarter(MakeFakeEnding(6, 0, m2::PointD(-0.001, 0), *m_graph), finish);
  vector<Segment> prevRoute;
  double weight = 0.0;
  TEST_EQUAL(CalculateRoute(*m_starter, prevRoute, weight), Algorithm::Result::OK, ());
  TestGeometry(*m_starter, prevRoute,
               {{-0.001, 0}, {0, 0}, {0.001, 0}, {0.001, 0.001}, {0.002, 0.001}, {0.002, 0.002}, {0.002, 0.003}});

  // The car has passed the service road by the detour. Going back to the service road is faster
  // than the rest of the detour only without the penalty of the turn off the service road.
  auto const newStart = MakeFakeEnding(3, 0, m2::PointD(0.0014, 0), *m_graph);
  FakeEdgesContainer const prevFakeEdges(std::move(*m_starter));
  IndexGraphStarter starter(newStart, FakeEnding{}, prevFakeEdges.GetNumFakeEdges(), false /* strictForward */,
                            *m_graph);
  starter.Append(prevFakeEdges);

  vector<SegmentEdge> prevEdges;
  for (size_t i = 0; i < prevRoute.size(); ++i)
  {
    prevEdges.emplace_back(prevRoute[i], i == 0 ? starter.CalcSegmentWeight(prevRoute[i], Purpose::Weight)
                                                : starter.CalcRouteEdgeWeight(prevRoute[i - 1], prevRoute[i]));
  }

  Algorithm::ParamsForTests<AdjustLengthChecker> params(starter, starter.GetStartSegment(), {} /* finalVertex */,
                                                        AdjustLengthChecker(starter));
  RoutingResult<Segment, RouteWeight> adjusted;
  TEST_EQUAL(Algorithm().AdjustRoute(params, prevEdges, adjusted), Algorithm::Result::OK, ());

  // The route goes on by the detour with the weight of a new search.
  vector<m2::PointD> const expectedGeom = {{0.0014, 0}, {0.0025, 0}, {0.002, 0.001}, {0.002, 0.002}, {0.002, 0.003}};
  TestGeometry(starter, adjusted.m_path, expectedGeom);

  SetStarter(newStart, finish);
  vector<Segment> newRoute;
  TEST_EQUAL(CalculateRoute(*m_starter, newRoute, weight), Algorithm::Result::OK, ());
  TestGeometry(*m_starter, newRoute, expectedGeom);
  TEST_ALMOST_EQUAL_ABS(adjusted.m_distance.GetWeight(), weight, kWeightEps, ());
}
}  // namespace turn_penalty_tests
