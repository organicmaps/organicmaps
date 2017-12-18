#pragma once

#include "routing/features_road_graph.hpp"
#include "routing/road_graph.hpp"

#include "routing_common/car_model.hpp"

#include "geometry/point2d.hpp"

#include <cstddef>
#include <memory>
#include <vector>

class Index;

namespace openlr
{
// TODO(mgsergio): Inherit from FeaturesRoadGraph.
class Graph
{
public:
  using Edge = routing::Edge;
  using EdgeVector = routing::FeaturesRoadGraph::TEdgeVector;
  using Junction = routing::Junction;

  Graph(Index const & index, std::shared_ptr<routing::CarModelFactory> carModelFactory);

  void GetOutgoingEdges(routing::Junction const & junction, EdgeVector & edges) const;
  void GetIngoingEdges(routing::Junction const & junction, EdgeVector & edges) const;

  void GetRegularIngoingEdges(Junction const & junction, EdgeVector & edges) const;
  void GetRegularOutgoingEdges(Junction const & junction, EdgeVector & edges) const;

  void FindClosestEdges(m2::PointD const & point, uint32_t count,
                        std::vector<pair<Edge, Junction>> & vicinities) const;

  void AddFakeEdges(Junction const & junction,
                    std::vector<pair<Edge, Junction>> const & vicinities);

  void AddIngoingFakeEdge(Edge const & e);
  void AddOutgoingFakeEdge(Edge const & e);

  void ResetFakes() { m_graph.ResetFakes(); }

private:
  routing::FeaturesRoadGraph m_graph;
};
}  // namespace openlr
