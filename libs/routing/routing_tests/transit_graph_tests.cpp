#include "testing/testing.hpp"

#include "routing/routing_tests/index_graph_tools.hpp"

#include "routing/base/astar_algorithm.hpp"
#include "routing/fake_feature_ids.hpp"
#include "routing/routing_helpers.hpp"
#include "routing/transit_graph.hpp"
#include "routing/transit_world_graph.hpp"

#include "platform/location.hpp"

#include "geometry/mercator.hpp"

#include "base/geo_object_id.hpp"

#include <algorithm>
#include <map>
#include <string>
#include <string_view>

namespace transit_graph_tests
{
namespace
{
routing::transit::FeatureId constexpr kGateFeatureId = 117;

struct Fixture
{
  explicit Fixture(std::string_view graphJson = {})
    : m_estimator(routing::EdgeEstimator::Create(routing::VehicleType::Pedestrian, 60, routing::SpeedKMpH(3), nullptr,
                                                 nullptr, nullptr))
  {
    m_data.DeserializeFromJson(graphJson.empty() ? R"({
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
    })"
                                                 : graphJson,
                               {{base::GeoObjectId(10), {kGateFeatureId}}});
    if (!graphJson.empty())
    {
      m_data.Sort();
      m_data.CheckValidSortedUnique();
    }
    auto const & gates = m_data.GetGates();
    for (size_t i = 0; i < gates.size(); ++i)
    {
      auto const & gate = gates[i];
      routing::LatLonWithAltitude const roadPoint(mercator::ToLatLon(gate.GetPoint() - m2::PointD(0.00001, 0)), 0);
      routing::FakeEnding ending;
      ending.m_originJunction = routing::LatLonWithAltitude(mercator::ToLatLon(gate.GetPoint()), 0);
      ending.m_projections.emplace_back(routing::Segment(0, i, 0, true), true, roadPoint, roadPoint, roadPoint);
      m_endings.emplace(gate.GetOsmId(), ending);
    }
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

routing::Segment Gate(routing::TransitGraph const & graph, bool isEntrance, m2::PointD const & point = {})
{
  routing::GateAccessesT accesses;
  graph.GetGatesNear(point, 100, isEntrance, accesses);
  TEST_EQUAL(accesses.size(), 1, ());
  for (auto const & segment : graph.GetFakeEdges(accesses.front().m_entrySegment, isEntrance))
    if (graph.FindGate(segment))
      return segment;
  TEST(false, ("Missing gate segment"));
  return {};
}

class WorldGraphAdapter final : public routing::AStarGraph<routing::Segment, routing::SegmentEdge, routing::RouteWeight>
{
public:
  explicit WorldGraphAdapter(routing::TransitWorldGraph & world) : m_world(world) {}

  Weight HeuristicCostEstimate(Vertex const & from, Vertex const & to) override
  {
    return m_world.HeuristicCostEstimate(m_world.GetPoint(from, true), m_world.GetPoint(to, true));
  }

  void GetOutgoingEdgesList(routing::astar::VertexData<Vertex, Weight> const & data, EdgeListT & edges) override
  {
    edges.clear();
    m_world.GetEdgeList(data, true, false, false, edges);
  }

  void GetIngoingEdgesList(routing::astar::VertexData<Vertex, Weight> const & data, EdgeListT & edges) override
  {
    edges.clear();
    m_world.GetEdgeList(data, false, false, false, edges);
  }

  Weight GetAStarWeightEpsilon() override { return Weight(1e-6); }

private:
  routing::TransitWorldGraph & m_world;
};

class BudgetGraph final : public routing::AStarGraph<routing::Segment, routing::SegmentEdge, routing::RouteWeight>
{
public:
  static Vertex Node(uint32_t id) { return {0, id, 0, true}; }

  void AddEdge(uint32_t from, uint32_t to, Weight const & weight)
  {
    m_outgoing[Node(from)].emplace_back(Node(to), weight);
    m_ingoing[Node(to)].emplace_back(Node(from), weight);
  }

  Weight HeuristicCostEstimate(Vertex const &, Vertex const &) override { return Weight(0); }
  Weight GetAStarWeightEpsilon() override { return Weight(1e-6); }

  void GetOutgoingEdgesList(routing::astar::VertexData<Vertex, Weight> const & data, EdgeListT & edges) override
  {
    edges = m_outgoing[data.m_vertex];
  }

  void GetIngoingEdgesList(routing::astar::VertexData<Vertex, Weight> const & data, EdgeListT & edges) override
  {
    edges = m_ingoing[data.m_vertex];
  }

private:
  std::map<Vertex, EdgeListT> m_outgoing;
  std::map<Vertex, EdgeListT> m_ingoing;
};
}  // namespace

UNIT_TEST(TransitGraph_GateMetadataPreservesTraversal)
{
  std::unique_ptr<routing::TransitGraph> graph;
  std::unique_ptr<routing::TransitWorldGraph> world;
  {
    Fixture fixture;
    graph = fixture.MakeGraph();
    world = fixture.MakeWorldGraph();
  }
  // Cached metadata and gate costs must outlive the source graph data.
  for (bool const isEntrance : {true, false})
  {
    auto const segment = Gate(*graph, isEntrance);
    // A gate at its stop has identical endpoints in both directions.
    TEST_EQUAL(graph->GetJunction(segment, false), graph->GetJunction(segment, true), ());
    auto const info = world->GetTransitInfo(segment);
    TEST(info, ());
    TEST_EQUAL(info->GetGateSubway().m_isEntrance, isEntrance, ());
    TEST_EQUAL(info->GetGateSubway().m_featureId, kGateFeatureId, ());
    TEST_EQUAL(graph->CalcSegmentWeight(segment, routing::EdgeEstimator::Purpose::ETA).GetWeight(), 60.0, ());
    TEST_EQUAL(world->CalculateETAWithoutPenalty(segment), 60.0, ());
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

UNIT_TEST(TransitWorldGraph_StationWalkingUsesAlternativeFactors)
{
  using Purpose = routing::EdgeEstimator::Purpose;
  Fixture fixture;
  auto const graph = fixture.MakeGraph();
  auto world = fixture.MakeWorldGraph();
  std::vector<routing::Segment> walking = {Gate(*graph, true), Gate(*graph, false), fixture.Edge(3, 4)};
  for (bool const isEntrance : {true, false})
  {
    routing::GateAccessesT accesses;
    graph->GetGatesNear({0, 0}, 100, isEntrance, accesses);
    TEST_EQUAL(accesses.size(), 1, ());
    auto const projection = accesses.front().m_entrySegment;
    walking.push_back(projection);
    double const physicalTime =
        fixture.m_estimator->CalcOffroad(graph->GetJunction(projection, false).GetLatLon(),
                                         graph->GetJunction(projection, true).GetLatLon(), Purpose::ETA);
    TEST_EQUAL(world->CalcSegmentWeight(projection, Purpose::ETA).GetWeight(), physicalTime, ());
  }

  for (double const factor : {3.0, 1.0})
  {
    fixture.m_estimator->SetTransitAltFactors(factor, 200.0);
    for (auto const & segment : walking)
    {
      auto const eta = world->CalcSegmentWeight(segment, Purpose::ETA);
      auto const weight = world->CalcSegmentWeight(segment, Purpose::Weight);
      TEST_EQUAL(weight.GetWeight(), factor * eta.GetWeight(), (segment));
      TEST_EQUAL(weight.GetTransitTime(), 0.0, (segment));
      TEST_EQUAL(eta.GetTransitTime(), 0.0, (segment));
    }
    auto const ride = world->CalcSegmentWeight(fixture.Edge(2, 3), Purpose::Weight);
    TEST_EQUAL(ride.GetWeight(), 80.0, ());
    TEST_EQUAL(ride.GetTransitTime(), 80.0, ());
    TEST_EQUAL(world->CalculateETAWithoutPenalty(fixture.Edge(3, 4)), 40.0, ());
  }
}

UNIT_TEST(TransitWorldGraph_StationWalkingCountsTowardBudget)
{
  using Purpose = routing::EdgeEstimator::Purpose;
  Fixture fixture;
  auto const graph = fixture.MakeGraph();
  auto world = fixture.MakeWorldGraph();
  auto const gate = world->CalcSegmentWeight(Gate(*graph, true), Purpose::Weight);
  auto const transfer = world->CalcSegmentWeight(fixture.Edge(3, 4), Purpose::Weight);
  TEST(world->CheckLength(50.0 * gate, 0), ());
  TEST(!world->CheckLength(51.0 * gate, 0), ());
  TEST(world->CheckLength(51.0 * gate, 1000), ());
  TEST(!world->CheckLength(76.0 * transfer, 0), ());
  auto const ride = world->CalcSegmentWeight(fixture.Edge(1, 2), Purpose::Weight);
  TEST(world->CheckLength(100.0 * ride, 0), ());

  fixture.m_estimator->SetTransitAltFactors(3.0, 200.0);
  auto const altGate = world->CalcSegmentWeight(Gate(*graph, true), Purpose::Weight);
  TEST(world->CheckLength(16.0 * altGate, 0), ());
  TEST(!world->CheckLength(17.0 * altGate, 0), ());
}

UNIT_TEST(TransitWorldGraph_StationConnectionsHaveSymmetricSearchWeights)
{
  Fixture fixture;
  fixture.m_estimator->SetTransitAltFactors(3.0, 200.0);
  auto const graph = fixture.MakeGraph();
  auto world = fixture.MakeWorldGraph();
  auto const checkConnection = [&](routing::Segment const & from, routing::Segment const & to, double expected)
  {
    for (bool const isOutgoing : {true, false})
    {
      routing::WorldGraph::SegmentEdgeListT edges;
      routing::astar::VertexData const vertex(isOutgoing ? from : to, routing::RouteWeight(0));
      world->GetEdgeList(vertex, isOutgoing, false, false, edges);
      auto const target = isOutgoing ? to : from;
      auto const it =
          std::find_if(edges.begin(), edges.end(), [&](auto const & edge) { return edge.GetTarget() == target; });
      TEST(it != edges.end(), (from, to));
      TEST_EQUAL(it->GetWeight().GetWeight(), expected, (isOutgoing));
    }
  };
  checkConnection(fixture.Edge(2, 3), fixture.Edge(3, 4), 120.0);
  checkConnection(fixture.Edge(3, 4), fixture.Edge(4, 5), 345.5);
  checkConnection(Gate(*graph, true), fixture.Edge(1, 2), 350.0);
}

UNIT_TEST(TransitWorldGraph_BidirectionalSearchKeepsWalkingBudget)
{
  Fixture fixture;
  auto world = fixture.MakeWorldGraph();
  BudgetGraph graph;
  for (uint32_t i = 0; i < 300; ++i)
    graph.AddEdge(i, i + 1, routing::RouteWeight(12));

  // A winding walk between nearby endpoints exceeds the full walking budget.
  auto checkLength = [&](routing::RouteWeight const & weight) { return world->CheckLength(weight, 50); };
  TEST(checkLength(routing::RouteWeight(1800)), ());
  TEST(!checkLength(routing::RouteWeight(3600)), ());
  using Algorithm = routing::AStarAlgorithm<routing::Segment, routing::SegmentEdge, routing::RouteWeight>;
  Algorithm::ParamsForTests<decltype(checkLength)> params(graph, BudgetGraph::Node(0), BudgetGraph::Node(300),
                                                          std::move(checkLength));
  routing::RoutingResult<routing::Segment, routing::RouteWeight> result;
  TEST_EQUAL(Algorithm().FindPath(params, result), Algorithm::Result::NoPath, ());
  result.Clear();
  TEST_EQUAL(Algorithm().FindPathBidirectional(params, result), Algorithm::Result::NoPath, ());
  TEST(result.Empty(), ());
}

UNIT_TEST(TransitWorldGraph_BidirectionalSearchContinuesAfterRejectedCandidate)
{
  Fixture fixture;
  auto world = fixture.MakeWorldGraph();
  BudgetGraph graph;
  for (uint32_t i = 0; i < 300; ++i)
    graph.AddEdge(i, i + 1, routing::RouteWeight(12));
  // The cheaper walk fails the budget. Continue its valid prefix to a slower ride
  // whose physical transit time does not consume the walking budget.
  graph.AddEdge(174, 300, routing::RouteWeight(2000, 0, 0, 0, 2000));

  auto checkLength = [&](routing::RouteWeight const & weight) { return world->CheckLength(weight, 50); };
  using Algorithm = routing::AStarAlgorithm<routing::Segment, routing::SegmentEdge, routing::RouteWeight>;
  Algorithm::ParamsForTests<decltype(checkLength)> params(graph, BudgetGraph::Node(0), BudgetGraph::Node(300),
                                                          std::move(checkLength));
  routing::RoutingResult<routing::Segment, routing::RouteWeight> result;
  TEST_EQUAL(Algorithm().FindPathBidirectional(params, result), Algorithm::Result::OK, ());
  TEST_EQUAL(result.m_distance.GetWeight(), 174.0 * 12 + 2000, ());
  TEST_EQUAL(result.m_distance.GetTransitTime(), 2000.0, ());
  TEST(world->CheckLength(result.m_distance, 50), ());
  TEST_EQUAL(result.m_path.size(), 176, ());
  TEST_EQUAL(result.m_path[result.m_path.size() - 2], BudgetGraph::Node(174), ());
  TEST_EQUAL(result.m_path.back(), BudgetGraph::Node(300), ());

  size_t emitted = 0;
  auto const code = Algorithm().FindPathBidirectionalEx(params, [&](auto && route)
  {
    ++emitted;
    TEST(world->CheckLength(route.m_distance, 50), ());
    return false;
  });
  TEST_EQUAL(code, Algorithm::Result::OK, ());
  TEST_GREATER(emitted, 0, ());
}

UNIT_TEST(TransitWorldGraph_SearchSelectsFasterParallelLine)
{
  struct Case
  {
    std::string m_type;
    int m_fastRideTime;
    int m_interval;
  };
  for (auto const & test : {Case{"train", 100, 60}, Case{"air_service", 100, 60}, Case{"train", 0, 0}})
  {
    Fixture fixture(R"({
      "stops": [
        {"id": 1, "osm_id": "1", "line_ids": [1, 2], "point": {"x": 0, "y": 0}},
        {"id": 2, "osm_id": "2", "line_ids": [1, 2], "point": {"x": 0.1, "y": 0}}
      ],
      "edges": [
        {"stop1_id": 1, "stop2_id": 2, "weight": 120, "line_id": 1, "transfer": false},
        {"stop1_id": 1, "stop2_id": 2, "weight": )" +
                    std::to_string(test.m_fastRideTime) + R"(,
         "line_id": 2, "transfer": false}
      ],
      "gates": [
        {"osm_id": "1000001", "entrance": true, "exit": false, "weight": 60,
         "stop_ids": [1], "point": {"x": 0, "y": 0}},
        {"osm_id": "1000002", "entrance": false, "exit": true, "weight": 60,
         "stop_ids": [2], "point": {"x": 0.1, "y": 0}}
      ],
      "lines": [
        {"id": 1, "interval": )" +
                    std::to_string(test.m_interval) + R"(, "network_id": 1, "number": "A",
         "stop_ids": [1, 2], "type": ")" +
                    test.m_type + R"(", "color": "red"},
        {"id": 2, "interval": )" +
                    std::to_string(test.m_interval) + R"(, "network_id": 1, "number": "B",
         "stop_ids": [1, 2], "type": ")" +
                    test.m_type + R"(", "color": "blue"}
      ],
      "networks": [{"id": 1, "title": "Parallel transit"}]
    })");
    auto const graph = fixture.MakeGraph();
    auto world = fixture.MakeWorldGraph();
    auto const start = Gate(*graph, true);
    auto const finish = Gate(*graph, false, {0.1, 0});
    WorldGraphAdapter adapter(*world);
    using Algorithm = routing::AStarAlgorithm<routing::Segment, routing::SegmentEdge, routing::RouteWeight>;
    size_t negativeReweights = 0;
    Algorithm::ParamsForTests<> params(adapter, start, finish);
    params.m_badReducedWeight = [&](auto const &, auto const &)
    {
      ++negativeReweights;
      return false;
    };
    routing::RoutingResult<routing::Segment, routing::RouteWeight> result;
    TEST_EQUAL(Algorithm().FindPathBidirectional(params, result), Algorithm::Result::OK, ());
    TEST_EQUAL(result.m_path.size(), 3, ());
    TEST_EQUAL(result.m_path.front(), start, ());
    TEST_EQUAL(result.m_path.back(), finish, ());

    routing::RouteWeight edgeSum;
    for (size_t i = 1; i < result.m_path.size(); ++i)
    {
      routing::WorldGraph::SegmentEdgeListT edges;
      routing::astar::VertexData const vertex(result.m_path[i - 1], edgeSum);
      world->GetEdgeList(vertex, true, false, false, edges);
      auto const edge =
          std::find_if(edges.begin(), edges.end(), [&](auto const & e) { return e.GetTarget() == result.m_path[i]; });
      TEST(edge != edges.end(), ());
      edgeSum += edge->GetWeight();
    }
    auto const info = world->GetTransitInfo(result.m_path[1]);
    TEST(info && info->GetType() == routing::TransitInfo::Type::Edge, ());
    TEST_EQUAL(info->GetEdgeSubway().m_lineId, 2,
               (result.m_distance.GetWeight(), edgeSum.GetWeight(), negativeReweights, test.m_type));
    // The search starts on the entrance segment, so only the ride, boarding wait and exit are charged.
    double const expected = test.m_fastRideTime + test.m_interval / 2.0 + 60;
    TEST_EQUAL(result.m_distance.GetWeight(), expected, ());
    TEST_EQUAL(edgeSum.GetWeight(), expected, ());
    TEST_EQUAL(negativeReweights, 0, ());
  }
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
