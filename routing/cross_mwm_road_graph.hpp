#pragma once

#include "car_router.hpp"
#include "osrm_engine.hpp"
#include "router.hpp"

#include "indexer/index.hpp"

#include "geometry/latlon.hpp"
#include "geometry/point2d.hpp"

#include "base/macros.hpp"

#include "std/functional.hpp"
#include "std/unordered_map.hpp"

namespace routing
{
/// OSRM graph node representation with graph mwm name and border crossing point.
struct CrossNode
{
  NodeID node;
  NodeID reverseNode;
  Index::MwmId mwmId;
  ms::LatLon point;
  bool isVirtual;

  CrossNode(NodeID node, NodeID reverse, Index::MwmId const & id, ms::LatLon const & point)
      : node(node), reverseNode(reverse), mwmId(id), point(point), isVirtual(false)
  {
  }

  CrossNode(NodeID node, Index::MwmId const & id, ms::LatLon const & point)
      : node(node), reverseNode(INVALID_NODE_ID), mwmId(id), point(point), isVirtual(false)
  {
  }

  CrossNode() : node(INVALID_NODE_ID), reverseNode(INVALID_NODE_ID), point(ms::LatLon::Zero()) {}

  inline bool IsValid() const { return node != INVALID_NODE_ID; }

  inline bool operator==(CrossNode const & a) const
  {
    return node == a.node && mwmId == a.mwmId && isVirtual == a.isVirtual;
  }

  inline bool operator<(CrossNode const & a) const
  {
    if (a.node != node)
      return node < a.node;

    if (isVirtual != a.isVirtual)
      return isVirtual < a.isVirtual;

    return mwmId < a.mwmId;
  }
};

inline string DebugPrint(CrossNode const & t)
{
  ostringstream out;
  out << "CrossNode [ node: " << t.node << ", map: " << t.mwmId.GetInfo()->GetCountryName()<< " ]";
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
  // Cashing wrapper for the ConstructBorderCrossImpl function.
  vector<BorderCross> const & ConstructBorderCross(OutgoingCrossNode const & startNode,
                                                   TRoutingMappingPtr const & currentMapping) const;

  // Pure function to construct boder cross by outgoing cross node.
  bool ConstructBorderCrossImpl(OutgoingCrossNode const & startNode,
                                TRoutingMappingPtr const & currentMapping,
                                vector<BorderCross> & cross) const;
  /*!
   * Adds a virtual edge to the graph so that it is possible to represent
   * the final segment of the path that leads from the map's border
   * to finalNode. Addition of such virtual edges for the starting node is
   * inlined elsewhere.
   */
  void AddVirtualEdge(IngoingCrossNode const & node, CrossNode const & finalNode,
                      EdgeWeight weight);

  map<CrossNode, vector<CrossWeightedEdge> > m_virtualEdges;

  mutable RoutingIndexManager m_indexManager;

  // Caching stuff.
  using TCachingKey = pair<TWrittenNodeId, Index::MwmId>;

  struct Hash
  {
    size_t operator()(TCachingKey const & p) const
    {
      return hash<TWrittenNodeId>()(p.first) ^ hash<string>()(p.second.GetInfo()->GetCountryName());
    }
  };

  mutable unordered_map<TCachingKey, vector<BorderCross>, Hash> m_cachedNextNodes;
};

//--------------------------------------------------------------------------------------------------
// Helper functions.
//--------------------------------------------------------------------------------------------------

/*!
 * \brief Convertor from CrossMwmGraph to cross mwm route task.
 * \warning It's assumed that the first and the last BorderCrosses are always virtual and represents
 * routing inside mwm.
 */
void ConvertToSingleRouterTasks(vector<BorderCross> const & graphCrosses,
                                FeatureGraphNode const & startGraphNode,
                                FeatureGraphNode const & finalGraphNode, TCheckedPath & route);

}  // namespace routing
