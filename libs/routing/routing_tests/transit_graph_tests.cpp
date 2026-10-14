#include "testing/testing.hpp"

#include "routing/routing_tests/index_graph_tools.hpp"

#include "routing/fake_feature_ids.hpp"
#include "routing/transit_graph.hpp"
#include "routing/transit_world_graph.hpp"

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
        {"id": 2, "osm_id": "2", "line_ids": [1], "point": {"x": 0.001, "y": 0}}
      ],
      "edges": [
        {"stop1_id": 1, "stop2_id": 2, "weight": 120, "line_id": 1, "transfer": false}
      ],
      "gates": [{"osm_id": "10", "entrance": true, "exit": true, "weight": 60,
                  "stop_ids": [1], "point": {"x": 0, "y": 0}}],
      "lines": [
        {"id": 1, "interval": 60, "network_id": 1, "number": "1", "stop_ids": [1, 2],
         "type": "subway", "color": "red"}
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
}  // namespace transit_graph_tests
