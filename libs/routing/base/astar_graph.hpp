#pragma once

#include "routing/base/astar_vertex_data.hpp"
#include "routing/base/astar_weight.hpp"
#include "routing/base/small_list.hpp"

#include "base/buffer_vector.hpp"

#include <map>
#include <vector>

#include "3party/skarupke/bytell_hash_map.hpp"

namespace routing
{
template <typename VertexType, typename EdgeType, typename WeightType>
class AStarGraph
{
public:
  using Vertex = VertexType;
  using Edge = EdgeType;
  using Weight = WeightType;

  using Parents = ska::bytell_hash_map<Vertex, Vertex>;

  using EdgeListT = SmallList<Edge>;

  virtual Weight HeuristicCostEstimate(Vertex const & from, Vertex const & to) = 0;

  virtual void GetOutgoingEdgesList(astar::VertexData<Vertex, Weight> const & vertexData, EdgeListT & edges) = 0;
  virtual void GetIngoingEdgesList(astar::VertexData<Vertex, Weight> const & vertexData, EdgeListT & edges) = 0;

  virtual void SetAStarParents(bool forward, Parents & parents);
  virtual void DropAStarParents();
  virtual bool AreWavesConnectible(Parents & forwardParents, Vertex const & commonVertex, Parents & backwardParents);

  virtual Weight GetAStarWeightEpsilon();

  virtual ~AStarGraph() = default;
};

template <typename VertexType, typename EdgeType, typename WeightType>
void AStarGraph<VertexType, EdgeType, WeightType>::SetAStarParents(bool /* forward */, Parents & /* parents */)
{}

template <typename VertexType, typename EdgeType, typename WeightType>
void AStarGraph<VertexType, EdgeType, WeightType>::DropAStarParents()
{}

template <typename VertexType, typename EdgeType, typename WeightType>
bool AStarGraph<VertexType, EdgeType, WeightType>::AreWavesConnectible(AStarGraph::Parents & /* forwardParents */,
                                                                       Vertex const & /* commonVertex */,
                                                                       AStarGraph::Parents & /* backwardParents */)
{
  return true;
}

template <typename VertexType, typename EdgeType, typename WeightType>
WeightType AStarGraph<VertexType, EdgeType, WeightType>::GetAStarWeightEpsilon()
{
  return routing::GetAStarWeightEpsilon<WeightType>();
}
}  // namespace routing
