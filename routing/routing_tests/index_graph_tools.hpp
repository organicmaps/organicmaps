#pragma once

#include "routing/edge_estimator.hpp"
#include "routing/index_graph.hpp"
#include "routing/index_graph_loader.hpp"
#include "routing/index_graph_starter.hpp"
#include "routing/num_mwm_id.hpp"
#include "routing/restrictions_serialization.hpp"
#include "routing/road_point.hpp"
#include "routing/segment.hpp"

#include "routing/base/astar_algorithm.hpp"

#include "traffic/traffic_info.hpp"

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
  void Init(unique_ptr<WorldGraph> graph) { m_graph = move(graph); }
  void SetStarter(IndexGraphStarter::FakeVertex const & start,
                  IndexGraphStarter::FakeVertex const & finish)
  {
    m_starter = make_unique<IndexGraphStarter>(start, finish, *m_graph);
  }

  void SetRestrictions(RestrictionVec && restrictions)
  {
    m_graph->GetIndexGraph(kTestNumMwmId).SetRestrictions(move(restrictions));
  }

  unique_ptr<WorldGraph> m_graph;
  unique_ptr<IndexGraphStarter> m_starter;
};

class TestGeometryLoader final : public routing::GeometryLoader
{
public:
  // GeometryLoader overrides:
  void Load(uint32_t featureId, routing::RoadGeometry & road) const override;

  void AddRoad(uint32_t featureId, bool oneWay, float speed,
               routing::RoadGeometry::Points const & points);

private:
  unordered_map<uint32_t, routing::RoadGeometry> m_roads;
};

class TestIndexGraphLoader final : public IndexGraphLoader
{
public:
  // IndexGraphLoader overrides:
  IndexGraph & GetIndexGraph(NumMwmId mwmId) override;
  virtual void Clear() override;

  void AddGraph(NumMwmId mwmId, unique_ptr<IndexGraph> graph);

private:
  unordered_map<NumMwmId, unique_ptr<IndexGraph>> m_graphs;
};

// A simple class to test graph algorithms for the index graph
// that do not depend on road geometry (but may depend on the
// lengths of roads).
class TestIndexGraphTopology
{
public:
  struct EdgeRequest
  {
    uint32_t m_src = 0;
    uint32_t m_dst = 0;
    double m_weight = 0.0;

    EdgeRequest(uint32_t src, uint32_t dst, double weight)
      : m_src(src), m_dst(dst), m_weight(weight)
    {
    }
  };

  // Creates an empty graph on |numVertices| vertices.
  TestIndexGraphTopology(uint32_t numVertices);

  // Adds a weighted directed edge to the graph. Multi-edges are not supported.
  // *NOTE* The edges are added lazily, i.e. edge requests are only stored here
  // and the graph itself is built only after a call to FindPath.
  void AddDirectedEdge(uint32_t vertexId1, uint32_t vertexId2, double weight);

  // Finds a path between the start and finish vertices. Returns true iff a path exists.
  bool FindPath(uint32_t start, uint32_t finish, double & pathWeight,
                vector<pair<uint32_t, uint32_t>> & pathEdges);

private:
  unique_ptr<WorldGraph> PrepareIndexGraph();
  void ClearGraphInternal();
  void BuildJoints();
  void BuildGraphFromRequests();
  void BuildSegmentFromEdge(uint32_t edgeId, uint32_t vertexId1, uint32_t vertexId2, double weight);

  uint32_t m_numVertices;
  map<pair<uint32_t, uint32_t>, double> m_edgeWeights;
  map<Segment, double> m_segmentWeights;
  map<Segment, pair<uint32_t, uint32_t>> m_segmentToEdge;
  map<uint32_t, vector<Segment>> m_outgoingSegments;
  map<uint32_t, vector<Segment>> m_incomingSegments;
  vector<Joint> m_joints;

  vector<EdgeRequest> m_edgeRequests;
};

unique_ptr<WorldGraph> BuildWorldGraph(unique_ptr<TestGeometryLoader> loader,
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

void TestRouteTime(IndexGraphStarter & starter,
                   AStarAlgorithm<IndexGraphStarter>::Result expectedRouteResult,
                   double expectedTime);

/// \brief Applies |restrictions| to graph in |restrictionTest| and
/// tests the resulting route.
/// \note restrictionTest should have a valid |restrictionTest.m_graph|.
void TestRestrictions(vector<m2::PointD> const & expectedRouteGeom,
                      AStarAlgorithm<IndexGraphStarter>::Result expectedRouteResult,
                      routing::IndexGraphStarter::FakeVertex const & start,
                      routing::IndexGraphStarter::FakeVertex const & finish,
                      RestrictionVec && restrictions, RestrictionTest & restrictionTest);
}  // namespace routing_test
