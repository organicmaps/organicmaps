#pragma once

#include "routing/features_road_graph.hpp"
#include "routing/road_graph.hpp"

#include "routing_common/car_model.hpp"

#include "geometry/point2d.hpp"

#include <cstddef>
#include <map>
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

  // Appends edges such as that edge.GetStartJunction() == junction to the |edges|.
  void GetOutgoingEdges(routing::Junction const & junction, EdgeVector & edges);
  // Appends edges such as that edge.GetEndJunction() == junction to the |edges|.
  void GetIngoingEdges(routing::Junction const & junction, EdgeVector & edges);

  // Appends edges such as that edge.GetStartJunction() == junction and edge.IsFake() == false
  // to the |edges|.
  void GetRegularOutgoingEdges(Junction const & junction, EdgeVector & edges);
  // Appends edges such as that edge.GetEndJunction() == junction and edge.IsFale() == false
  // to the |edges|.
  void GetRegularIngoingEdges(Junction const & junction, EdgeVector & edges);

  void FindClosestEdges(m2::PointD const & point, uint32_t const count,
                        std::vector<pair<Edge, Junction>> & vicinities) const;

  void AddFakeEdges(Junction const & junction,
                    std::vector<pair<Edge, Junction>> const & vicinities);

  void AddIngoingFakeEdge(Edge const & e);
  void AddOutgoingFakeEdge(Edge const & e);

  void ResetFakes() { m_graph.ResetFakes(); }

private:
  routing::FeaturesRoadGraph m_graph;
  std::map<Junction, EdgeVector> m_outgoingCache;
  std::map<Junction, EdgeVector> m_ingoingCache;
};
}  // namespace openlr
