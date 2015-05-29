#pragma once

#include "osrm_engine.hpp"
#include "osrm_router.hpp"
#include "router.hpp"

#include "base/graph.hpp"

#include "indexer/index.hpp"

namespace routing
{
/// OSRM graph node representation with graph mwm name and border crossing point.
struct CrossNode
{
  NodeID node;
  string mwmName;
  m2::PointD point;

  CrossNode(NodeID node, string const & mwmName, m2::PointD const & point)
      : node(node), mwmName(mwmName), point(point)
  {
  }
  CrossNode() : node(INVALID_NODE_ID), point(m2::PointD::Zero()) {}

  inline bool IsValid() const { return node != INVALID_NODE_ID; }
};

inline bool operator==(CrossNode const & a, CrossNode const & b)
{
  return a.node == b.node && a.mwmName == b.mwmName;
}

inline bool operator<(CrossNode const & a, CrossNode const & b)
{
  if (a.node < b.node)
    return true;
  else if (a.node == b.node)
    return a.mwmName < b.mwmName;
  return false;
}

inline string DebugPrint(CrossNode const & t)
{
  ostringstream out;
  out << t.mwmName << " " << t.node;
  return out.str();
}

/// Representation of border crossing. Contains node on previous map and node on next map.
using TCrossPair = pair<CrossNode, CrossNode>;

inline bool operator==(TCrossPair const & a, TCrossPair const & b) { return a.first == b.first; }

inline bool operator<(TCrossPair const & a, TCrossPair const & b) { return a.first < b.first; }

inline string DebugPrint(TCrossPair const & t)
{
  ostringstream out;
  out << DebugPrint(t.first) << " to " << DebugPrint(t.second) << "\n";
  return out.str();
}

/// A class which represents an cross mwm weighted edge used by CrossMwmGraph.
class CrossWeightedEdge
{
public:
  explicit CrossWeightedEdge(TCrossPair const & target, double weight) : target(target), weight(weight) {}

  inline TCrossPair const & GetTarget() const { return target; }
  inline double GetWeight() const { return weight; }

private:
  TCrossPair target;
  double weight;
};

/// A graph used for cross mwm routing.
class CrossMwmGraph : public Graph<TCrossPair, CrossWeightedEdge, CrossMwmGraph>
{
public:
  explicit CrossMwmGraph(RoutingIndexManager & indexManager) : m_indexManager(indexManager) {}

  IRouter::ResultCode SetStartNode(CrossNode const & startNode);
  IRouter::ResultCode SetFinalNode(CrossNode const & finalNode);

private:
  friend class Graph<TCrossPair, CrossWeightedEdge, CrossMwmGraph>;

  TCrossPair FindNextMwmNode(OutgoingCrossNode const & startNode,
                             TRoutingMappingPtr const & currentMapping) const;

  // Graph<CrossNode, CrossWeightedEdge, CrossMwmGraph> implementation:
  void GetOutgoingEdgesListImpl(TCrossPair const & v, vector<CrossWeightedEdge> & adj) const;
  void GetIngoingEdgesListImpl(TCrossPair const & v, vector<CrossWeightedEdge> & adj) const
  {
    ASSERT(!"IMPL", ());
  }

  double HeuristicCostEstimateImpl(TCrossPair const & v, TCrossPair const & w) const;

  map<CrossNode, vector<CrossWeightedEdge> > m_virtualEdges;
  mutable RoutingIndexManager m_indexManager;
};
}  // namespace routing
