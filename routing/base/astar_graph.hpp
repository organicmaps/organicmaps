#pragma once

#include <vector>

namespace routing
{
template <typename VertexType, typename EdgeType, typename WeightType>
class AStarGraph
{
public:

  using Vertex = VertexType;
  using Edge = EdgeType;
  using Weight = WeightType;

  virtual Weight HeuristicCostEstimate(Vertex const & from, Vertex const & to) = 0;

  virtual void GetOutgoingEdgesList(Vertex const & v, std::vector<Edge> & edges) = 0;
  virtual void GetIngoingEdgesList(Vertex const & v, std::vector<Edge> & edges) = 0;

  virtual ~AStarGraph() = default;
};
}  // namespace routing
