#pragma once

#include "osrm_engine.hpp"
#include "osrm_router.hpp"
#include "router.hpp"

#include "indexer/index.hpp"

#include "base/graph.hpp"

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
  if (a.node != b.node)
    return a.node < b.node;
  return a.mwmName < b.mwmName;
}

inline string DebugPrint(CrossNode const & t)
{
  ostringstream out;
  out << "CrossNode [ node: " << t.node << ", map: " << t.mwmName << " ]";
  return out.str();
}

/// Representation of border crossing. Contains node on previous map and node on next map.
struct BorderCross
{
  CrossNode fromNode;
  CrossNode toNode;

  BorderCross(CrossNode const & from, CrossNode const & to) : fromNode(from), toNode(to) {}
  BorderCross() : fromNode(), toNode() {}
};

inline bool operator==(BorderCross const & a, BorderCross const & b) { return a.toNode == b.toNode; }

inline bool operator<(BorderCross const & a, BorderCross const & b) { return a.toNode < b.toNode; }

inline string DebugPrint(BorderCross const & t)
{
  ostringstream out;
  out << "Border cross from: " << DebugPrint(t.fromNode) << " to: " << DebugPrint(t.toNode) << "\n";
  return out.str();
}

/// A class which represents an cross mwm weighted edge used by CrossMwmGraph.
class CrossWeightedEdge
{
public:
  CrossWeightedEdge(BorderCross const & target, double weight) : target(target), weight(weight) {}

  inline BorderCross const & GetTarget() const { return target; }
  inline double GetWeight() const { return weight; }

private:
  BorderCross target;
  double weight;
};

/// A graph used for cross mwm routing.
class CrossMwmGraph : public Graph<BorderCross, CrossWeightedEdge, CrossMwmGraph>
{
public:
  explicit CrossMwmGraph(RoutingIndexManager & indexManager) : m_indexManager(indexManager) {}

  IRouter::ResultCode SetStartNode(CrossNode const & startNode);
  IRouter::ResultCode SetFinalNode(CrossNode const & finalNode);

private:
  friend class Graph<BorderCross, CrossWeightedEdge, CrossMwmGraph>;

  BorderCross FindNextMwmNode(OutgoingCrossNode const & startNode,
                             TRoutingMappingPtr const & currentMapping) const;

  // Graph<BorderCross, CrossWeightedEdge, CrossMwmGraph> implementation:
  void GetOutgoingEdgesListImpl(BorderCross const & v, vector<CrossWeightedEdge> & adj) const;
  void GetIngoingEdgesListImpl(BorderCross const & v, vector<CrossWeightedEdge> & adj) const
  {
    ASSERT(!"IMPL", ());
  }

  double HeuristicCostEstimateImpl(BorderCross const & v, BorderCross const & w) const;

  map<CrossNode, vector<CrossWeightedEdge> > m_virtualEdges;
  mutable RoutingIndexManager m_indexManager;
};
}  // namespace routing
