#pragma once

#include <map>
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

  using Parents = std::map<Vertex, Vertex>;

  virtual Weight HeuristicCostEstimate(Vertex const & from, Vertex const & to) = 0;

  virtual void GetOutgoingEdgesList(Vertex const & v, std::vector<Edge> & edges) = 0;
  virtual void GetIngoingEdgesList(Vertex const & v, std::vector<Edge> & edges) = 0;

  virtual void SetAStarParents(bool forward, Parents & parents);
  virtual void DropAStarParents();
  virtual bool AreWavesConnectible(Parents & forwardParents, Vertex const & commonVertex,
                                   Parents & backwardParents);

  virtual ~AStarGraph() = default;
};

template <typename VertexType, typename EdgeType, typename WeightType>
void AStarGraph<VertexType, EdgeType, WeightType>::SetAStarParents(bool /* forward */,
                                                                   std::map<Vertex, Vertex> & /* parents */) {}

template <typename VertexType, typename EdgeType, typename WeightType>
void AStarGraph<VertexType, EdgeType, WeightType>::DropAStarParents() {}

template <typename VertexType, typename EdgeType, typename WeightType>
bool AStarGraph<VertexType, EdgeType, WeightType>::AreWavesConnectible(AStarGraph::Parents & /* forwardParents */,
                                                                       Vertex const & /* commonVertex */,
                                                                       AStarGraph::Parents & /* backwardParents */)
{
  return true;
}
}  // namespace routing
