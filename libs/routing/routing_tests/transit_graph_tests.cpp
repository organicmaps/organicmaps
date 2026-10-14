#include "testing/testing.hpp"

#include "routing/routing_tests/index_graph_tools.hpp"

#include "routing/fake_feature_ids.hpp"
#include "routing/routing_helpers.hpp"
#include "routing/transit_graph.hpp"
#include "routing/transit_world_graph.hpp"

#include "platform/location.hpp"

#include "geometry/mercator.hpp"

namespace transit_graph_tests
{
namespace
{
struct Fixture
{
  Fixture()
    : m_estimator(routing::EdgeEstimator::Create(routing::VehicleType::Pedestrian, 60, routing::SpeedKMpH(3), nullptr,
                                                 nullptr, nullptr))
  {
    m_data.DeserializeFromJson(R"({
      "stops": [
        {"id": 1, "osm_id": "1", "line_ids": [1], "point": {"x": 0, "y": 0}},
        {"id": 2, "osm_id": "2", "line_ids": [1], "point": {"x": 0.001, "y": 0}},
        {"id": 3, "osm_id": "3", "line_ids": [1, 2], "point": {"x": 0.002, "y": 0}},
        {"id": 4, "osm_id": "4", "line_ids": [2], "point": {"x": 0.003, "y": 0}},
        {"id": 5, "osm_id": "5", "line_ids": [2, 3], "point": {"x": 0.004, "y": 0}},
        {"id": 6, "osm_id": "6", "line_ids": [3], "point": {"x": 0.005, "y": 0}}
      ],
      "edges": [
        {"stop1_id": 1, "stop2_id": 2, "weight": 120, "line_id": 1, "transfer": false},
        {"stop1_id": 2, "stop2_id": 3, "weight": 80, "line_id": 1, "transfer": false},
        {"stop1_id": 3, "stop2_id": 4, "weight": 40, "line_id": 4294967295, "transfer": true},
        {"stop1_id": 4, "stop2_id": 5, "weight": 100, "line_id": 2, "transfer": false},
        {"stop1_id": 3, "stop2_id": 5, "weight": 110, "line_id": 2, "transfer": false},
        {"stop1_id": 5, "stop2_id": 6, "weight": 90, "line_id": 3, "transfer": false}
      ],
      "gates": [{"osm_id": "10", "entrance": true, "exit": true, "weight": 60,
                  "stop_ids": [1], "point": {"x": 0, "y": 0}}],
      "lines": [
        {"id": 1, "interval": 60, "network_id": 1, "number": "1", "stop_ids": [1, 2, 3],
         "type": "subway", "color": "red"},
        {"id": 2, "interval": 91, "network_id": 1, "number": "2", "stop_ids": [3, 4, 5],
         "type": "subway", "color": "green"},
        {"id": 3, "interval": 0, "network_id": 1, "number": "3", "stop_ids": [5, 6],
         "type": "subway", "color": "blue"}
      ],
      "networks": [{"id": 1, "title": "Transit test"}]
    })",
                               {});
    routing::LatLonWithAltitude const roadPoint(mercator::ToLatLon({-0.00001, 0}), 0);
    routing::FakeEnding ending;
    ending.m_originJunction = routing::LatLonWithAltitude(mercator::ToLatLon({0, 0}), 0);
    ending.m_projections.emplace_back(routing::Segment(0, 0, 0, true), true, roadPoint, roadPoint, roadPoint);
    m_endings.emplace(m_data.GetGates().front().GetOsmId(), ending);
  }

  std::unique_ptr<routing::TransitGraph> MakeGraph(routing::NumMwmId mwmId = 0)
  {
    auto graph = std::make_unique<routing::TransitGraph>(mwmId, m_estimator);
    graph->Fill(m_data, m_endings);
    return graph;
  }

  std::unique_ptr<routing::TransitWorldGraph> MakeWorldGraph()
  {
    auto indexLoader = std::make_unique<routing_test::TestIndexGraphLoader>();
    auto transitLoader = std::make_unique<routing_test::TestTransitGraphLoader>();
    for (routing::NumMwmId mwmId : {0, 1})
    {
      indexLoader->AddGraph(mwmId, std::make_unique<routing::IndexGraph>());
      transitLoader->AddGraph(mwmId, MakeGraph(mwmId));
    }
    return std::make_unique<routing::TransitWorldGraph>(nullptr, std::move(indexLoader), std::move(transitLoader),
                                                        m_estimator);
  }

  routing::Segment Edge(routing::transit::StopId from, routing::transit::StopId to, routing::NumMwmId mwmId = 0) const
  {
    auto const & edges = m_data.GetEdges();
    for (size_t i = 0; i < edges.size(); ++i)
      if (edges[i].GetStop1Id() == from && edges[i].GetStop2Id() == to)
        return routing::Segment(mwmId, routing::FakeFeatureIds::kTransitGraphFeaturesStart + i, 0, true);
    TEST(false, ("Missing edge", from, to));
    return {};
  }

  std::shared_ptr<routing::EdgeEstimator> m_estimator;
  routing::transit::GraphData m_data;
  routing::TransitGraph::Endings m_endings;
};

routing::Segment Gate(routing::TransitGraph const & graph, bool isEntrance)
{
  routing::GateAccessesT accesses;
  graph.GetGatesNear({0, 0}, 100, isEntrance, accesses);
  TEST_EQUAL(accesses.size(), 1, ());
  for (auto const & segment : graph.GetFakeEdges(accesses.front().m_entrySegment, isEntrance))
    if (graph.FindGate(segment))
      return segment;
  TEST(false, ("Missing gate segment"));
  return {};
}
}  // namespace

UNIT_TEST(TransitGraph_GateMetadataPreservesTraversal)
{
  Fixture fixture;
  auto const graph = fixture.MakeGraph();
  auto world = fixture.MakeWorldGraph();
  for (bool const isEntrance : {true, false})
  {
    auto const segment = Gate(*graph, isEntrance);
    // A gate at its stop has identical endpoints in both directions.
    TEST_EQUAL(graph->GetJunction(segment, false), graph->GetJunction(segment, true), ());
    TEST_EQUAL(graph->FindGate(segment)->m_isEntrance, isEntrance, ());
    auto const info = world->GetTransitInfo(segment);
    TEST(info, ());
    TEST_EQUAL(info->GetGateSubway().m_isEntrance, isEntrance, ());
  }
}

UNIT_TEST(TransitWorldGraph_ETAIncludesBoardingWaits)
{
  Fixture fixture;
  auto const graph = fixture.MakeGraph();
  auto world = fixture.MakeWorldGraph();
  auto const gate = Gate(*graph, true);
  auto const ride1 = fixture.Edge(1, 2);
  auto const ride2 = fixture.Edge(2, 3);
  auto const transfer = fixture.Edge(3, 4);
  auto const ride3 = fixture.Edge(4, 5);
  auto const directChange = fixture.Edge(3, 5);
  auto const zeroWait = fixture.Edge(5, 6);
  TEST_EQUAL(world->CalculateETA(gate, ride1, 0), 150.0, ());
  TEST_EQUAL(world->CalculateETA(ride1, ride2, 0), 80.0, ());
  TEST_EQUAL(world->CalculateETA(ride2, transfer, 0), 40.0, ());
  TEST_EQUAL(world->CalculateETA(transfer, ride3, 0), 145.5, ());
  TEST_EQUAL(world->CalculateETA(ride2, directChange, 0), 155.5, ());
  TEST_EQUAL(world->CalculateETA(ride3, zeroWait, 0), 90.0, ());
  TEST_EQUAL(world->CalculateETAWithoutPenalty(ride1), 120.0, ());
  TEST_EQUAL(world->GetTransitInfo(ride1)->GetEdgeSubway().m_rideTimeSec, 120, ());

  fixture.m_estimator->SetTransitAltFactors(3.0, 200.0);
  TEST_EQUAL(world->CalculateETA(gate, ride1, 0), 150.0, ());
  TEST_EQUAL(world->CalculateETA(transfer, ride3, 0), 145.5, ());
  TEST_EQUAL(world->CalculateETA(ride3, zeroWait, 0), 90.0, ());
  auto const penalty = graph->GetTransferPenalty(graph->GetRideLineId(ride3), zeroWait);
  TEST_EQUAL(penalty.GetWeight(), 200.0, ());
  TEST_EQUAL(penalty.GetTransitTime(), 0.0, ());
}

UNIT_TEST(TransitWorldGraph_CrossMwmETAKeepsThePreviousLine)
{
  Fixture fixture;
  auto world = fixture.MakeWorldGraph();
  TEST_EQUAL(world->CalculateETA(fixture.Edge(1, 2, 1), fixture.Edge(2, 3), 0), 80.0, ());
  TEST_EQUAL(world->CalculateETA(fixture.Edge(2, 3, 1), fixture.Edge(3, 5), 0), 155.5, ());
}

UNIT_TEST(TransitStarter_ETAIncludesWaitAfterFakeConnection)
{
  Fixture fixture;
  auto world = fixture.MakeWorldGraph();
  routing::FakeEnding ending;
  auto const ride = fixture.Edge(1, 2);
  ending.m_originJunction = world->GetJunction(ride, false);
  routing::IndexGraphStarter starter(ending, ending, 0, false, *world);
  TEST_EQUAL(starter.CalculateETA(starter.GetStartSegment(), ride, 0), 150.0, ());
}

UNIT_TEST(TransitRoute_BoardingWaitPrecedesRideProgress)
{
  for (double const waitSec : {0.0, 30.0, 45.5, 600.0})
  {
    std::vector<m2::PointD> const points = {{0, 0}, {0, 0}, {0.01, 0}, {0.01, 0}};
    std::vector<routing::RouteSegment> segments;
    for (size_t i = 0; i < 3; ++i)
    {
      auto const segment =
          i == 1 ? routing::Segment(0, routing::FakeFeatureIds::kTransitGraphFeaturesStart, 0, true)
                 : routing::Segment(routing::kFakeNumMwmId, routing::FakeFeatureIds::kIndexGraphStarterId, i, true);
      segments.emplace_back(segment, routing::turns::TurnItem(), geometry::PointWithAltitude(points[i + 1], 0),
                            routing::RouteSegment::RoadNameInfo());
    }
    segments[1].SetTransitInfo(std::make_unique<routing::TransitInfo>(
        routing::transit::Edge(1, 2, 120, 1, false, std::vector<routing::transit::ShapeId>{})));
    routing::FillSegmentInfo({0, 120 + waitSec, 120 + waitSec}, segments);
    routing::Route route;
    route.SetRoutingSettings(routing::GetRoutingSettings(routing::VehicleType::Transit));
    route.SetGeometry(points.begin(), points.end());
    route.SetRouteSegments(std::move(segments));
    route.SetSubroutes(std::vector<routing::Route::SubrouteAttrs>{
        {geometry::PointWithAltitude(points.front(), 0), geometry::PointWithAltitude(points.back(), 0), 0, 3}});
    TEST_EQUAL(route.GetTotalTimeSec(), 120.0 + waitSec, ());
    TEST_EQUAL(route.GetCurrentTimeToEndSec(), 120.0 + waitSec, ());
    location::GpsInfo gps;
    gps.m_latitude = 0;
    gps.m_longitude = 0.005;
    gps.m_horizontalAccuracy = 1;
    TEST(route.MoveIterator(gps), ());
    TEST_ALMOST_EQUAL_ABS(route.GetCurrentTimeToEndSec(), 60.0, 1e-5, ());
    gps.m_longitude = 0.01;
    TEST(route.MoveIterator(gps), ());
    TEST_ALMOST_EQUAL_ABS(route.GetCurrentTimeToEndSec(), 0.0, 1e-5, ());
  }
}
}  // namespace transit_graph_tests
