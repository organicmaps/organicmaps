#pragma once

#include "osrm_engine.hpp"
#include "osrm_router.hpp"
#include "router.hpp"

#include "indexer/index.hpp"

#include "geometry/point2d.hpp"

#include "base/macros.hpp"

#include "std/unordered_map.hpp"

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

  inline bool operator==(CrossNode const & a) const
  {
    return node == a.node && mwmName == a.mwmName;
  }

  inline bool operator<(CrossNode const & a) const
  {
    if (a.node != node)
      return node < a.node;
    return mwmName < a.mwmName;
  }
};

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
  BorderCross() = default;

  inline bool operator==(BorderCross const & a) const { return toNode == a.toNode; }
  inline bool operator<(BorderCross const & a) const { return toNode < a.toNode; }
};

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

/// A graph used for cross mwm routing in an astar algorithms.
class CrossMwmGraph
{
public:
  using TVertexType = BorderCross;
  using TEdgeType = CrossWeightedEdge;

  explicit CrossMwmGraph(RoutingIndexManager & indexManager) : m_indexManager(indexManager) {}

  void GetOutgoingEdgesList(BorderCross const & v, vector<CrossWeightedEdge> & adj) const;
  void GetIngoingEdgesList(BorderCross const & /* v */,
                           vector<CrossWeightedEdge> & /* adj */) const
  {
    NOTIMPLEMENTED();
  }

  double HeuristicCostEstimate(BorderCross const & v, BorderCross const & w) const;

  IRouter::ResultCode SetStartNode(CrossNode const & startNode);
  IRouter::ResultCode SetFinalNode(CrossNode const & finalNode);

private:
  BorderCross FindNextMwmNode(OutgoingCrossNode const & startNode,
                              TRoutingMappingPtr const & currentMapping) const;

  map<CrossNode, vector<CrossWeightedEdge> > m_virtualEdges;
  mutable RoutingIndexManager m_indexManager;

  // hash function for unordered map realisation.
  mutable unordered_map<m2::PointD, BorderCross, m2::PointD::Hash> m_cachedNextNodes;
};
}  // namespace routing
