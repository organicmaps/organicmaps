#include "routing/osrm_engine.hpp"
#include "routing/osrm2feature_map.hpp"

#include "base/logging.hpp"
#include "base/timer.hpp"

#include "3party/osrm/osrm-backend/data_structures/internal_route_result.hpp"
#include "3party/osrm/osrm-backend/data_structures/search_engine_data.hpp"
#include "3party/osrm/osrm-backend/routing_algorithms/n_to_m_many_to_many.hpp"
#include "3party/osrm/osrm-backend/routing_algorithms/shortest_path.hpp"

namespace routing
{
bool IsRouteExist(InternalRouteResult const & r)
{
  return !(INVALID_EDGE_WEIGHT == r.shortest_path_length || r.segment_end_coordinates.empty() ||
           r.source_traversed_in_reverse.empty());
}

void GenerateRoutingTaskFromNodeId(NodeID const nodeId, bool const isStartNode,
                                   PhantomNode & taskNode)
{
  taskNode.forward_node_id = isStartNode ? nodeId : INVALID_NODE_ID;
  taskNode.reverse_node_id = isStartNode ? INVALID_NODE_ID : nodeId;
  taskNode.forward_weight = 0;
  taskNode.reverse_weight = 0;
  taskNode.forward_offset = 0;
  taskNode.reverse_offset = 0;
  taskNode.name_id = 1;
}

void FindWeightsMatrix(TRoutingNodes const & sources, TRoutingNodes const & targets,
                       TRawDataFacade & facade, vector<EdgeWeight> & result)
{
  SearchEngineData engineData;
  NMManyToManyRouting<TRawDataFacade> pathFinder(&facade, engineData);
  PhantomNodeArray sourcesTaskVector(sources.size());
  PhantomNodeArray targetsTaskVector(targets.size());
  for (size_t i = 0; i < sources.size(); ++i)
    sourcesTaskVector[i].push_back(sources[i].node);
  for (size_t i = 0; i < targets.size(); ++i)
    targetsTaskVector[i].push_back(targets[i].node);

  // Calculate time consumption of a NtoM path finding.
  my::HighResTimer timer(true);
  shared_ptr<vector<EdgeWeight>> resultTable = pathFinder(sourcesTaskVector, targetsTaskVector);
  LOG(LINFO, ("Duration of a single one-to-many routing call", timer.ElapsedNano(), "ns"));
  timer.Reset();
  ASSERT_EQUAL(resultTable->size(), sources.size() * targets.size(), ());
  result.swap(*resultTable);
}

bool FindSingleRoute(FeatureGraphNode const & source, FeatureGraphNode const & target,
                     TRawDataFacade & facade, RawRoutingResult & rawRoutingResult)
{
  SearchEngineData engineData;
  InternalRouteResult result;
  ShortestPathRouting<TRawDataFacade> pathFinder(&facade, engineData);
  PhantomNodes nodes;
  nodes.source_phantom = source.node;
  nodes.target_phantom = target.node;

  if ((nodes.source_phantom.forward_node_id != INVALID_NODE_ID ||
       nodes.source_phantom.reverse_node_id != INVALID_NODE_ID) &&
      (nodes.target_phantom.forward_node_id != INVALID_NODE_ID ||
       nodes.target_phantom.reverse_node_id != INVALID_NODE_ID))
  {
    result.segment_end_coordinates.push_back(nodes);
    pathFinder({nodes}, {}, result);
  }

  if (IsRouteExist(result))
  {
    rawRoutingResult.sourceEdge = source;
    rawRoutingResult.targetEdge = target;
    rawRoutingResult.shortestPathLength = result.shortest_path_length;
    for (auto const & path : result.unpacked_path_segments)
    {
      vector<RawPathData> data;
      data.reserve(path.size());
      for (auto const & element : path)
      {
        data.emplace_back(element.node, element.segment_duration);
      }
      rawRoutingResult.unpackedPathSegments.emplace_back(move(data));
    }
    return true;
  }

  return false;
}

FeatureGraphNode::FeatureGraphNode(NodeID const nodeId, bool const isStartNode,
                                   Index::MwmId const & id)
    : segmentPoint(m2::PointD::Zero()), mwmId(id)
{
  node.forward_node_id = isStartNode ? nodeId : INVALID_NODE_ID;
  node.reverse_node_id = isStartNode ? INVALID_NODE_ID : nodeId;
  node.forward_weight = 0;
  node.reverse_weight = 0;
  node.forward_offset = 0;
  node.reverse_offset = 0;
  node.name_id = 1;
  segment.m_fid = kInvalidFid;
}

FeatureGraphNode::FeatureGraphNode(NodeID const nodeId, NodeID const reverseNodeId,
                                   bool const isStartNode, Index::MwmId const & id)
    : segmentPoint(m2::PointD::Zero()), mwmId(id)
{
  node.forward_node_id = isStartNode ? nodeId : reverseNodeId;
  node.reverse_node_id = isStartNode ? reverseNodeId : nodeId;
  node.forward_weight = 0;
  node.reverse_weight = 0;
  node.forward_offset = 0;
  node.reverse_offset = 0;
  node.name_id = 1;
  segment.m_fid = kInvalidFid;
}

FeatureGraphNode::FeatureGraphNode()
{
  node.forward_node_id = INVALID_NODE_ID;
  node.reverse_node_id = INVALID_NODE_ID;
  node.forward_weight = 0;
  node.reverse_weight = 0;
  node.forward_offset = 0;
  node.reverse_offset = 0;
  node.name_id = 1;
  segment.m_fid = kInvalidFid;
  segmentPoint = m2::PointD::Zero();
}

}  // namespace routing
