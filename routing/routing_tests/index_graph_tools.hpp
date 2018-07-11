#pragma once

#include "routing/edge_estimator.hpp"
#include "routing/fake_ending.hpp"
#include "routing/index_graph.hpp"
#include "routing/index_graph_loader.hpp"
#include "routing/index_graph_starter.hpp"
#include "routing/restrictions_serialization.hpp"
#include "routing/road_access.hpp"
#include "routing/road_point.hpp"
#include "routing/segment.hpp"
#include "routing/single_vehicle_world_graph.hpp"
#include "routing/transit_graph_loader.hpp"
#include "routing/transit_world_graph.hpp"

#include "routing/base/astar_algorithm.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "traffic/traffic_info.hpp"

#include "transit/transit_types.hpp"

#include "indexer/classificator_loader.hpp"

#include "geometry/point2d.hpp"

#include "std/algorithm.hpp"
#include "std/cstdint.hpp"
#include "std/map.hpp"
#include "std/shared_ptr.hpp"
#include "std/unique_ptr.hpp"
#include "std/unordered_map.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

namespace routing_test
{
using namespace routing;

// The value doesn't matter, it doesn't associated with any real mwm id.
// It just a noticeable value to detect the source of such id while debuggging unit tests.
NumMwmId constexpr kTestNumMwmId = 777;

struct RestrictionTest
{
  RestrictionTest() { classificator::Load(); }
  void Init(unique_ptr<SingleVehicleWorldGraph> graph) { m_graph = move(graph); }
  void SetStarter(FakeEnding const & start, FakeEnding const & finish);
  void SetRestrictions(RestrictionVec && restrictions)
  {
    m_graph->GetIndexGraphForTests(kTestNumMwmId).SetRestrictions(move(restrictions));
  }

  unique_ptr<SingleVehicleWorldGraph> m_graph;
  unique_ptr<IndexGraphStarter> m_starter;
};

class TestGeometryLoader final : public routing::GeometryLoader
{
public:
  // GeometryLoader overrides:
  ~TestGeometryLoader() override = default;

  void Load(uint32_t featureId, routing::RoadGeometry & road) override;

  void AddRoad(uint32_t featureId, bool oneWay, float speed,
               routing::RoadGeometry::Points const & points);

  void SetPassThroughAllowed(uint32_t featureId, bool passThroughAllowed);

private:
  unordered_map<uint32_t, routing::RoadGeometry> m_roads;
};

class ZeroGeometryLoader final : public routing::GeometryLoader
{
public:
  // GeometryLoader overrides:
  ~ZeroGeometryLoader() override = default;

  void Load(uint32_t featureId, routing::RoadGeometry & road) override;
};

class TestIndexGraphLoader final : public IndexGraphLoader
{
public:
  // IndexGraphLoader overrides:
  ~TestIndexGraphLoader() override = default;

  Geometry & GetGeometry(NumMwmId mwmId) override { return GetIndexGraph(mwmId).GetGeometry(); }
  IndexGraph & GetIndexGraph(NumMwmId mwmId) override;
  void Clear() override;

  void AddGraph(NumMwmId mwmId, unique_ptr<IndexGraph> graph);

private:
  unordered_map<NumMwmId, unique_ptr<IndexGraph>> m_graphs;
};

class TestTransitGraphLoader : public TransitGraphLoader
{
public:
  // TransitGraphLoader overrides:
  ~TestTransitGraphLoader() override = default;

  TransitGraph & GetTransitGraph(NumMwmId mwmId, IndexGraph & indexGraph) override;
  void Clear() override;

  void AddGraph(NumMwmId mwmId, unique_ptr<TransitGraph> graph);

private:
  unordered_map<NumMwmId, unique_ptr<TransitGraph>> m_graphs;
};

// An estimator that uses the information from the supported |segmentWeights| map
// and completely ignores road geometry. The underlying graph is assumed
// to be directed, i.e. it is not guaranteed that each segment has its reverse
// in the map.
class WeightedEdgeEstimator final : public EdgeEstimator
{
public:
  // maxSpeedKMpH doesn't matter, but should be greater then any road speed in all tests.
  // offroadSpeedKMpH doesn't matter, but should be > 0 and <= maxSpeedKMpH.
  explicit WeightedEdgeEstimator(map<Segment, double> const & segmentWeights)
    : EdgeEstimator(1e10 /* maxSpeedKMpH */, 1.0 /* offroadSpeedKMpH */)
    , m_segmentWeights(segmentWeights)
  {
  }

  // EdgeEstimator overrides:
  ~WeightedEdgeEstimator() override = default;

  double CalcSegmentWeight(Segment const & segment, RoadGeometry const & /* road */) const override;
  double CalcSegmentETA(Segment const & segment, RoadGeometry const & road) const override { return 0.0; }
  double GetUTurnPenalty() const override;
  bool LeapIsAllowed(NumMwmId /* mwmId */) const override;

private:
  map<Segment, double> m_segmentWeights;
};

// A simple class to test graph algorithms for the index graph
// that do not depend on road geometry (but may depend on the
// lengths of roads).
class TestIndexGraphTopology final
{
public:
  using Vertex = uint32_t;
  using Edge = pair<Vertex, Vertex>;

  // Creates an empty graph on |numVertices| vertices.
  TestIndexGraphTopology(uint32_t numVertices);

  // Adds a weighted directed edge to the graph. Multi-edges are not supported.
  // *NOTE* The edges are added lazily, i.e. edge requests are only stored here
  // and the graph itself is built only after a call to FindPath.
  void AddDirectedEdge(Vertex from, Vertex to, double weight);

  // Sets access for previously added edge.
  void SetEdgeAccess(Vertex from, Vertex to, RoadAccess::Type type);

  // Sets access type for previously added point.
  void SetVertexAccess(Vertex v, RoadAccess::Type type);

  // Finds a path between the start and finish vertices. Returns true iff a path exists.
  bool FindPath(Vertex start, Vertex finish, double & pathWeight, vector<Edge> & pathEdges) const;

private:
  struct EdgeRequest
  {
    uint32_t m_id = 0;
    Vertex m_from = 0;
    Vertex m_to = 0;
    double m_weight = 0.0;
    // Access type for edge.
    RoadAccess::Type m_accessType = RoadAccess::Type::Yes;
    // Access type for vertex from.
    RoadAccess::Type m_fromAccessType = RoadAccess::Type::Yes;
    // Access type for vertex to.
    RoadAccess::Type m_toAccessType = RoadAccess::Type::Yes;

    EdgeRequest(uint32_t id, Vertex from, Vertex to, double weight)
      : m_id(id), m_from(from), m_to(to), m_weight(weight)
    {
    }
  };

  // Builder builds a graph from edge requests.
  struct Builder
  {
    Builder(uint32_t numVertices) : m_numVertices(numVertices) {}
    unique_ptr<SingleVehicleWorldGraph> PrepareIndexGraph();
    void BuildJoints();
    void BuildGraphFromRequests(vector<EdgeRequest> const & requests);
    void BuildSegmentFromEdge(EdgeRequest const & request);

    uint32_t const m_numVertices;
    map<Edge, double> m_edgeWeights;
    map<Segment, double> m_segmentWeights;
    map<Segment, Edge> m_segmentToEdge;
    map<Vertex, vector<Segment>> m_outgoingSegments;
    map<Vertex, vector<Segment>> m_ingoingSegments;
    vector<Joint> m_joints;
    RoadAccess m_roadAccess;
  };

  void AddDirectedEdge(vector<EdgeRequest> & edgeRequests, Vertex from, Vertex to,
                       double weight) const;

  uint32_t const m_numVertices;
  vector<EdgeRequest> m_edgeRequests;
};

unique_ptr<SingleVehicleWorldGraph> BuildWorldGraph(unique_ptr<TestGeometryLoader> loader,
                                                    shared_ptr<EdgeEstimator> estimator,
                                                    vector<Joint> const & joints);
unique_ptr<SingleVehicleWorldGraph> BuildWorldGraph(unique_ptr<ZeroGeometryLoader> loader,
                                                    shared_ptr<EdgeEstimator> estimator,
                                                    vector<Joint> const & joints);

routing::Joint MakeJoint(vector<routing::RoadPoint> const & points);

shared_ptr<routing::EdgeEstimator> CreateEstimatorForCar(
    traffic::TrafficCache const & trafficCache);
shared_ptr<routing::EdgeEstimator> CreateEstimatorForCar(shared_ptr<TrafficStash> trafficStash);

routing::AStarAlgorithm<routing::IndexGraphStarter>::Result CalculateRoute(
    routing::IndexGraphStarter & starter, vector<routing::Segment> & roadPoints, double & timeSec);

void TestRouteGeometry(
    routing::IndexGraphStarter & starter,
    routing::AStarAlgorithm<routing::IndexGraphStarter>::Result expectedRouteResult,
    vector<m2::PointD> const & expectedRouteGeom);

/// \brief Applies |restrictions| to graph in |restrictionTest| and
/// tests the resulting route.
/// \note restrictionTest should have a valid |restrictionTest.m_graph|.
void TestRestrictions(vector<m2::PointD> const & expectedRouteGeom,
                      AStarAlgorithm<IndexGraphStarter>::Result expectedRouteResult,
                      FakeEnding const & start, FakeEnding const & finish,
                      RestrictionVec && restrictions, RestrictionTest & restrictionTest);

// Tries to find a unique path from |from| to |to| in |graph|.
// If |expectedPathFound| is true, |expectedWeight| and |expectedEdges| must
// specify the weight and edges of the unique shortest path.
// If |expectedPathFound| is false, |expectedWeight| and |expectedEdges| may
// take arbitrary values.
void TestTopologyGraph(TestIndexGraphTopology const & graph, TestIndexGraphTopology::Vertex from,
                       TestIndexGraphTopology::Vertex to, bool expectedPathFound,
                       double const expectedWeight,
                       vector<TestIndexGraphTopology::Edge> const & expectedEdges);

// Creates FakeEnding projected to |Segment(kTestNumMwmId, featureId, segmentIdx, true /* forward
// */)|.
FakeEnding MakeFakeEnding(uint32_t featureId, uint32_t segmentIdx, m2::PointD const & point,
                          WorldGraph & graph);

std::unique_ptr<IndexGraphStarter> MakeStarter(FakeEnding const & start, FakeEnding const & finish,
                                               WorldGraph & graph);
}  // namespace routing_test
