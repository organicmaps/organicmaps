#pragma once

#include "std/utility.hpp"
#include "std/vector.hpp"

namespace routing
{
template <typename TVertex, typename TEdge, typename TImpl>
class Graph
{
public:
  using TVertexType = TVertex;
  using TEdgeType = TEdge;

  void GetAdjacencyList(TVertexType const & v, vector<TEdgeType> & adj) const
  {
    return GetImpl().GetAdjacencyListImpl(v, adj);
  }

  double HeuristicCostEstimate(TVertexType const & v, TVertexType const & w) const
  {
    return GetImpl().HeuristicCostEstimateImpl(v, w);
  }

private:
  TImpl & GetImpl() { return static_cast<TImpl &>(*this); }

  TImpl const & GetImpl() const { return static_cast<TImpl const &>(*this); }
};
}  // namespace routing
