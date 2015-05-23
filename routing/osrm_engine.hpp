#pragma once

#include "routing/osrm2feature_map.hpp"
#include "routing/osrm_data_facade.hpp"

#include "geometry/point2d.hpp"

#include "std/vector.hpp"

#include "3party/osrm/osrm-backend/data_structures/query_edge.hpp"

namespace routing
{
/// Single graph node representation for routing task
struct FeatureGraphNode
{
  PhantomNode m_node;
  OsrmMappingTypes::FtSeg m_seg;
  m2::PointD m_segPt;

  /*!
  * \brief GenerateRoutingTaskFromNodeId fill taskNode with values for making route
  * \param nodeId osrm node idetifier
  * \param isStartNode true if this node will first in the path
  * \param taskNode output point task for router
  */
  FeatureGraphNode(NodeID const nodeId, bool const isStartNode);

  /// \brief Invalid graph node constructor
  FeatureGraphNode();
};

/*!
 * \brief The RawPathData struct is our representation of OSRM PathData struct.
 * I use it for excluding dependency from OSRM. Contains OSRM node ID and it's weight.
 */
struct RawPathData
{
  NodeID node;
  EdgeWeight segment_duration;

  RawPathData() : node(SPECIAL_NODEID), segment_duration(INVALID_EDGE_WEIGHT) {}

  RawPathData(NodeID node, EdgeWeight segment_duration)
      : node(node), segment_duration(segment_duration)
  {
  }
};

/*!
 * \brief The OSRM routing result struct. Contains the routing result, it's cost and source and
 * target edges.
 * \property m_shortestPathLength Length of a founded route.
 * \property m_unpackedPathSegments Segments of a founded route.
 * \property m_sourceEdge Source graph node of a route.
 * \property m_targetEdge Target graph node of a route.
 */
struct RawRoutingResult
{
  int m_shortestPathLength;
  vector<vector<RawPathData>> m_unpackedPathSegments;
  FeatureGraphNode m_sourceEdge;
  FeatureGraphNode m_targetEdge;
};

//@todo (dragunov) make proper name
using TRoutingNodes = vector<FeatureGraphNode>;
using TRawDataFacade = OsrmRawDataFacade<QueryEdge::EdgeData>;

/*!
   * \brief FindWeightsMatrix Find weights matrix from sources to targets. WARNING it finds only
 * weights, not pathes.
   * \param sources Sources graph nodes vector. Each source is the representation of a start OSRM
 * node.
   * \param targets Targets graph nodes vector. Each target is the representation of a finish OSRM
 * node.
   * \param facade Osrm data facade reference.
   * \param packed Result vector with weights. Source nodes are rows.
   * cost(source1 -> target1) cost(source1 -> target2) cost(source2 -> target1) cost(source2 ->
 * target2)
   */
void FindWeightsMatrix(TRoutingNodes const & sources, TRoutingNodes const & targets,
                       TRawDataFacade & facade, vector<EdgeWeight> & result);

/*! Find single shortest path in a single MWM between 2 OSRM nodes
   * \param source Source OSRM graph node to make path.
   * \param taget Target OSRM graph node to make path.
   * \param facade OSRM routing data facade to recover graph information.
   * \param rawRoutingResult Routing result structure.
   * \return true when path exists, false otherwise.
   */
bool FindSingleRoute(FeatureGraphNode const & source, FeatureGraphNode const & target,
                     TRawDataFacade & facade, RawRoutingResult & rawRoutingResult);

}  // namespace routing
