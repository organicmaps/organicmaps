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
  return !(INVALID_EDGE_WEIGHT == r.shortest_path_length ||
          r.segment_end_coordinates.empty() ||
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

void FindWeightsMatrix(const RoutingNodesT &sources, const RoutingNodesT &targets, RawDataFacadeT &facade, vector<EdgeWeight> &result)
{
  SearchEngineData engineData;
  NMManyToManyRouting<RawDataFacadeT> pathFinder(&facade, engineData);
  PhantomNodeArray sourcesTaskVector(sources.size());
  PhantomNodeArray targetsTaskVector(targets.size());
  for (int i = 0; i < sources.size(); ++i)
    sourcesTaskVector[i].push_back(sources[i].m_node);
  for (int i = 0; i < targets.size(); ++i)
    targetsTaskVector[i].push_back(targets[i].m_node);

  // Calculate time consumption of a NtoM path finding.
  my::HighResTimer timer(true);
  shared_ptr<vector<EdgeWeight>> resultTable = pathFinder(sourcesTaskVector, targetsTaskVector);
  LOG(LINFO, ("Duration of a single one-to-many routing call", timer.ElapsedNano(), "ns"));
  timer.Reset();
  ASSERT_EQUAL(resultTable->size(), sources.size() * targets.size(), ());
  result.swap(*resultTable);
}

bool FindSingleRoute(const FeatureGraphNode &source, const FeatureGraphNode &target, RawDataFacadeT &facade, RawRoutingResult &rawRoutingResult)
{
  SearchEngineData engineData;
  InternalRouteResult result;
  ShortestPathRouting<RawDataFacadeT> pathFinder(&facade, engineData);
  PhantomNodes nodes;
  nodes.source_phantom = source.m_node;
  nodes.target_phantom = target.m_node;

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
    rawRoutingResult.m_sourceEdge = source;
    rawRoutingResult.m_targetEdge = target;
    rawRoutingResult.m_shortestPathLength = result.shortest_path_length;
    for (auto const & path: result.unpacked_path_segments)
    {
      vector<RawPathData> data;
      data.reserve(path.size());
      for (auto const & element : path)
      {
        data.emplace_back(RawPathData(element.node, element.segment_duration));
      }
      rawRoutingResult.unpacked_path_segments.emplace_back(move(data));
    }
    return true;
  }

  return false;
}

FeatureGraphNode::FeatureGraphNode(const NodeID nodeId, const bool isStartNode)
{
  m_node.forward_node_id = isStartNode ? nodeId : INVALID_NODE_ID;
  m_node.reverse_node_id = isStartNode ? INVALID_NODE_ID : nodeId;
  m_node.forward_weight = 0;
  m_node.reverse_weight = 0;
  m_node.forward_offset = 0;
  m_node.reverse_offset = 0;
  m_node.name_id = 1;
  m_seg.m_fid = OsrmMappingTypes::FtSeg::INVALID_FID;
  m_segPt = m2::PointD::Zero();
}

FeatureGraphNode::FeatureGraphNode()
{
  m_node.forward_node_id = INVALID_NODE_ID;
  m_node.reverse_node_id = INVALID_NODE_ID;
  m_node.forward_weight = 0;
  m_node.reverse_weight = 0;
  m_node.forward_offset = 0;
  m_node.reverse_offset = 0;
  m_node.name_id = 1;
  m_seg.m_fid = OsrmMappingTypes::FtSeg::INVALID_FID;
  m_segPt = m2::PointD::Zero();
}

} // namespace routing
