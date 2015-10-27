#include "cross_mwm_road_graph.hpp"
#include "cross_mwm_router.hpp"

#include "geometry/distance_on_sphere.hpp"

namespace
{
inline bool IsValidEdgeWeight(EdgeWeight const & w) { return w != INVALID_EDGE_WEIGHT; }
}

namespace routing
{
IRouter::ResultCode CrossMwmGraph::SetStartNode(CrossNode const & startNode)
{
  ASSERT(!startNode.mwmName.empty(), ());
  // TODO (ldragunov) make cancellation if necessary
  TRoutingMappingPtr startMapping = m_indexManager.GetMappingByName(startNode.mwmName);
  MappingGuard startMappingGuard(startMapping);
  UNUSED_VALUE(startMappingGuard);
  startMapping->LoadCrossContext();

  // Load source data.
  vector<OutgoingCrossNode> outgoingNodes;
  startMapping->m_crossContext.GetAllOutgoingNodes(outgoingNodes);
  size_t const outSize = outgoingNodes.size();
  // Can't find the route if there are no routes outside source map.
  if (!outSize)
    return IRouter::RouteNotFound;

  // Generate routing task from one source to several targets.
  TRoutingNodes sources(1), targets;
  targets.reserve(outSize);
  for (auto const & node : outgoingNodes)
  {
    targets.emplace_back(node.m_nodeId, false /* isStartNode */, startNode.mwmName);
    targets.back().segmentPoint = MercatorBounds::FromLatLon(node.m_point);
  }
  sources[0] = FeatureGraphNode(startNode.node, startNode.reverseNode, true /* isStartNode */,
                                startNode.mwmName);

  vector<EdgeWeight> weights;
  FindWeightsMatrix(sources, targets, startMapping->m_dataFacade, weights);
  if (find_if(weights.begin(), weights.end(), &IsValidEdgeWeight) == weights.end())
    return IRouter::StartPointNotFound;
  vector<CrossWeightedEdge> dummyEdges;
  for (size_t i = 0; i < outSize; ++i)
  {
    if (IsValidEdgeWeight(weights[i]))
    {
      BorderCross nextNode = FindNextMwmNode(outgoingNodes[i], startMapping);
      if (nextNode.toNode.IsValid())
        dummyEdges.emplace_back(nextNode, weights[i]);
    }
  }

  m_virtualEdges.insert(make_pair(startNode, dummyEdges));
  return IRouter::NoError;
}

void CrossMwmGraph::AddVirtualEdge(IngoingCrossNode const & node, CrossNode const & finalNode,
                                   EdgeWeight weight)
{
  CrossNode start(node.m_nodeId, finalNode.mwmName, node.m_point);
  vector<CrossWeightedEdge> dummyEdges;
  dummyEdges.emplace_back(BorderCross(finalNode, finalNode), weight);
  m_virtualEdges.insert(make_pair(start, dummyEdges));
}

IRouter::ResultCode CrossMwmGraph::SetFinalNode(CrossNode const & finalNode)
{
  ASSERT(finalNode.mwmName.length(), ());
  TRoutingMappingPtr finalMapping = m_indexManager.GetMappingByName(finalNode.mwmName);
  MappingGuard finalMappingGuard(finalMapping);
  UNUSED_VALUE(finalMappingGuard);
  finalMapping->LoadCrossContext();

  // Load source data.
  vector<IngoingCrossNode> ingoingNodes;
  finalMapping->m_crossContext.GetAllIngoingNodes(ingoingNodes);
  size_t const ingoingSize = ingoingNodes.size();
  // If there is no routes inside target map.
  if (ingoingSize == 0)
    return IRouter::RouteNotFound;

  // Generate routing task from one source to several targets.
  TRoutingNodes sources, targets(1);
  sources.reserve(ingoingSize);

  for (auto const & node : ingoingNodes)
  {
    // Case with a target node at the income mwm node.
    if (node.m_nodeId == finalNode.node)
    {
      AddVirtualEdge(node, finalNode, 0 /* no weight */);
      return IRouter::NoError;
    }
    sources.emplace_back(node.m_nodeId, true /* isStartNode */, finalNode.mwmName);
    sources.back().segmentPoint = MercatorBounds::FromLatLon(node.m_point);
  }
  vector<EdgeWeight> weights;

  targets[0] = FeatureGraphNode(finalNode.node, finalNode.reverseNode, false /* isStartNode */,
                                finalNode.mwmName);
  FindWeightsMatrix(sources, targets, finalMapping->m_dataFacade, weights);
  if (find_if(weights.begin(), weights.end(), &IsValidEdgeWeight) == weights.end())
    return IRouter::EndPointNotFound;
  for (size_t i = 0; i < ingoingSize; ++i)
  {
    if (IsValidEdgeWeight(weights[i]))
    {
      AddVirtualEdge(ingoingNodes[i], finalNode, weights[i]);
    }
  }
  return IRouter::NoError;
}

BorderCross CrossMwmGraph::FindNextMwmNode(OutgoingCrossNode const & startNode,
                                           TRoutingMappingPtr const & currentMapping) const
{
  ms::LatLon const & startPoint = startNode.m_point;

  // Check cached crosses.
  auto const it = m_cachedNextNodes.find(startPoint);
  if (it != m_cachedNextNodes.end())
  {
    return it->second;
  }

  string const & nextMwm = currentMapping->m_crossContext.GetOutgoingMwmName(startNode);
  TRoutingMappingPtr nextMapping;
  nextMapping = m_indexManager.GetMappingByName(nextMwm);
  // If we haven't this routing file, we skip this path.
  if (!nextMapping->IsValid())
    return BorderCross();
  nextMapping->LoadCrossContext();

  IngoingCrossNode ingoingNode;
  if (nextMapping->m_crossContext.FindIngoingNodeByPoint(startPoint, ingoingNode))
  {
    auto const & targetPoint = ingoingNode.m_point;
    BorderCross const cross(
        CrossNode(startNode.m_nodeId, currentMapping->GetCountryName(), targetPoint),
        CrossNode(ingoingNode.m_nodeId, nextMwm, targetPoint));
    m_cachedNextNodes.insert(make_pair(startPoint, cross));
    return cross;
  }

  return BorderCross();
}

void CrossMwmGraph::GetOutgoingEdgesList(BorderCross const & v,
                                         vector<CrossWeightedEdge> & adj) const
{
  // Check for virtual edges.
  adj.clear();
  auto const it = m_virtualEdges.find(v.toNode);
  if (it != m_virtualEdges.end())
  {
    adj.insert(adj.end(), it->second.begin(), it->second.end());
    return;
  }

  // Loading cross routing section.
  TRoutingMappingPtr currentMapping = m_indexManager.GetMappingByName(v.toNode.mwmName);
  ASSERT(currentMapping->IsValid(), ());
  currentMapping->LoadCrossContext();
  currentMapping->FreeFileIfPossible();

  CrossRoutingContextReader const & currentContext = currentMapping->m_crossContext;

  // Find income node.
  IngoingCrossNode ingoingNode;
  bool found = currentContext.FindIngoingNodeByPoint(v.toNode.point, ingoingNode);
  CHECK(found, ());
  if (ingoingNode.m_nodeId != v.toNode.node)
  {
    LOG(LDEBUG, ("Several nodes stores in one border point.", v.toNode.point));
    vector<IngoingCrossNode> ingoingNodes;
    currentContext.GetAllIngoingNodes(ingoingNodes);
    for(auto const & node : ingoingNodes)
    {
      if (node.m_nodeId == v.toNode.node)
      {
        ingoingNode = node;
        break;
      }
    }
  }

  vector<OutgoingCrossNode> outgoingNodes;
  currentContext.GetAllOutgoingNodes(outgoingNodes);

  // Find outs. Generate adjacency list.
  for (auto const & node : outgoingNodes)
  {
    EdgeWeight const outWeight = currentContext.GetAdjacencyCost(ingoingNode, node);
    if (outWeight != INVALID_CONTEXT_EDGE_WEIGHT && outWeight != 0)
    {
      BorderCross target = FindNextMwmNode(node, currentMapping);
      if (target.toNode.IsValid())
        adj.emplace_back(target, outWeight);
    }
  }
}

double CrossMwmGraph::HeuristicCostEstimate(BorderCross const & v, BorderCross const & w) const
{
  // Simple travel time heuristic works worse than simple Dijkstra's algorithm, represented by
  // always 0 heuristics estimation.
  return 0;
}

void ConvertToSingleRouterTasks(vector<BorderCross> const & graphCrosses,
                                FeatureGraphNode const & startGraphNode,
                                FeatureGraphNode const & finalGraphNode, TCheckedPath & route)
{
  route.clear();
  for (size_t i = 0; i + 1 < graphCrosses.size(); ++i)
  {
    ASSERT_EQUAL(graphCrosses[i].toNode.mwmName, graphCrosses[i + 1].fromNode.mwmName, ());
    route.emplace_back(graphCrosses[i].toNode.node,
                       MercatorBounds::FromLatLon(graphCrosses[i].toNode.point),
                       graphCrosses[i + 1].fromNode.node,
                       MercatorBounds::FromLatLon(graphCrosses[i + 1].fromNode.point),
                       graphCrosses[i].toNode.mwmName);
  }

  if (route.empty())
    return;

  route.front().startNode = startGraphNode;
  route.back().finalNode = finalGraphNode;
  ASSERT_EQUAL(route.front().startNode.mwmName, route.front().finalNode.mwmName, ());
  ASSERT_EQUAL(route.back().startNode.mwmName, route.back().finalNode.mwmName, ());
}

}  // namespace routing
