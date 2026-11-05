#include "testing/testing.hpp"

#include "generator/generator_tests_support/routing_helpers.hpp"

#include "traffic/traffic_cache.hpp"

#include "routing/routing_tests/index_graph_tools.hpp"

#include "routing/edge_estimator.hpp"
#include "routing/geometry.hpp"
#include "routing/index_graph.hpp"

#include "indexer/classificator_loader.hpp"

#include "geometry/point2d.hpp"

#include <algorithm>
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
unique_ptr<SingleVehicleWorldGraph> BuildJunctionGraph()
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
  return BuildWorldGraph(std::move(loader), CreateEstimatorForCar(trafficCache), joints);
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

// S --F6-- * --F0-- A ------F3------ E
//                   |              /
//                   +----F1----- C  F4      F1 service, the others residential
//                                |
//                                F2
//                                |
//                                * --F5-- F
// The detour F3, F4 is 69 m (5 s) longer than the service road F1 with two turns.
unique_ptr<SingleVehicleWorldGraph> BuildServiceShortcutGraph()
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
    loader->AddRoad(featureId, false /* oneWay */, 50.0 /* speed */, roads[featureId]);
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
    TestRouteGeometry(*m_starter, Algorithm::Result::OK, expectedGeom);

    // The service road is used when there is no other way.
    SetRestrictions({{0, 3}, {2, 4}});
    expectedGeom = {{-0.001, 0}, {0, 0}, {0.001, 0}, {0.001, 0.001}, {0.002, 0.001}, {0.002, 0.002}, {0.002, 0.003}};
    if (reverse)
      std::reverse(expectedGeom.begin(), expectedGeom.end());
    TestRouteGeometry(*m_starter, Algorithm::Result::OK, expectedGeom);
  }
}
}  // namespace turn_penalty_tests
