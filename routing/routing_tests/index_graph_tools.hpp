#pragma once

#include "routing/edge_estimator.hpp"
#include "routing/index_graph.hpp"
#include "routing/index_graph_starter.hpp"
#include "routing/restrictions_serialization.hpp"
#include "routing/road_point.hpp"
#include "routing/segment.hpp"

#include "routing/base/astar_algorithm.hpp"

#include "traffic/traffic_info.hpp"

#include "indexer/classificator_loader.hpp"

#include "geometry/point2d.hpp"

#include "std/algorithm.hpp"
#include "std/shared_ptr.hpp"
#include "std/unique_ptr.hpp"
#include "std/unordered_map.hpp"
#include "std/vector.hpp"

namespace routing_test
{
using namespace routing;

struct RestrictionTest
{
  RestrictionTest() { classificator::Load(); }
  void Init(unique_ptr<IndexGraph> graph) { m_graph = move(graph); }
  void SetStarter(IndexGraphStarter::FakeVertex const & start,
                  IndexGraphStarter::FakeVertex const & finish)
  {
    m_starter = make_unique<IndexGraphStarter>(*m_graph, start, finish);
  }

  void SetRestrictions(RestrictionVec && restrictions)
  {
    m_graph->SetRestrictions(move(restrictions));
  }

  unique_ptr<IndexGraph> m_graph;
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

routing::Joint MakeJoint(vector<routing::RoadPoint> const & points);

shared_ptr<routing::EdgeEstimator> CreateEstimator(traffic::TrafficCache const & trafficCache);

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
