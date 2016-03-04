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
  ASSERT(startNode.mwmId.IsAlive(), ());
  // TODO (ldragunov) make cancellation if necessary
  TRoutingMappingPtr startMapping = m_indexManager.GetMappingById(startNode.mwmId);
  if (!startMapping->IsValid())
      return IRouter::ResultCode::StartPointNotFound;
  MappingGuard startMappingGuard(startMapping);
  UNUSED_VALUE(startMappingGuard);
  startMapping->LoadCrossContext();

  // Load source data.
  vector<OutgoingCrossNode> outgoingNodes;

  startMapping->m_crossContext.ForEachOutgoingNode([&outgoingNodes](OutgoingCrossNode const & node)
                                                   {
                                                     outgoingNodes.push_back(node);
                                                   });

  size_t const outSize = outgoingNodes.size();
  // Can't find the route if there are no routes outside source map.
  if (!outSize)
    return IRouter::RouteNotFound;

  // Generate routing task from one source to several targets.
  TRoutingNodes sources(1), targets;
  targets.reserve(outSize);
  for (auto const & node : outgoingNodes)
  {
    targets.emplace_back(node.m_nodeId, false /* isStartNode */, startNode.mwmId);
    targets.back().segmentPoint = MercatorBounds::FromLatLon(node.m_point);
  }
  sources[0] = FeatureGraphNode(startNode.node, startNode.reverseNode, true /* isStartNode */,
                                startNode.mwmId);

  vector<EdgeWeight> weights;
  FindWeightsMatrix(sources, targets, startMapping->m_dataFacade, weights);
  if (find_if(weights.begin(), weights.end(), &IsValidEdgeWeight) == weights.end())
    return IRouter::StartPointNotFound;
  vector<CrossWeightedEdge> dummyEdges;
  for (size_t i = 0; i < outSize; ++i)
  {
    if (IsValidEdgeWeight(weights[i]))
    {
      vector<BorderCross> const & nextCrosses = ConstructBorderCross(outgoingNodes[i], startMapping);
      for (auto const & nextCross : nextCrosses)
      {
        if (nextCross.toNode.IsValid())
          dummyEdges.emplace_back(nextCross, weights[i]);
      }
    }
  }

  m_virtualEdges.insert(make_pair(startNode, dummyEdges));
  return IRouter::NoError;
}

void CrossMwmGraph::AddVirtualEdge(IngoingCrossNode const & node, CrossNode const & finalNode,
                                   EdgeWeight weight)
{
  CrossNode start(node.m_nodeId, finalNode.mwmId, node.m_point);
  vector<CrossWeightedEdge> dummyEdges;
  dummyEdges.emplace_back(BorderCross(finalNode, finalNode), weight);
  m_virtualEdges.insert(make_pair(start, dummyEdges));
}

IRouter::ResultCode CrossMwmGraph::SetFinalNode(CrossNode const & finalNode)
{
  ASSERT(finalNode.mwmId.IsAlive(), ());
  TRoutingMappingPtr finalMapping = m_indexManager.GetMappingById(finalNode.mwmId);
  if (!finalMapping->IsValid())
      return IRouter::ResultCode::EndPointNotFound;
  MappingGuard finalMappingGuard(finalMapping);
  UNUSED_VALUE(finalMappingGuard);
  finalMapping->LoadCrossContext();

  // Load source data.
  vector<IngoingCrossNode> ingoingNodes;
  finalMapping->m_crossContext.ForEachIngoingNode([&ingoingNodes](IngoingCrossNode const & node)
                                                   {
                                                     ingoingNodes.push_back(node);
                                                   });
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
    sources.emplace_back(node.m_nodeId, true /* isStartNode */, finalNode.mwmId);
    sources.back().segmentPoint = MercatorBounds::FromLatLon(node.m_point);
  }
  vector<EdgeWeight> weights;

  targets[0] = FeatureGraphNode(finalNode.node, finalNode.reverseNode, false /* isStartNode */,
                                finalNode.mwmId);
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

vector<BorderCross> const & CrossMwmGraph::ConstructBorderCross(OutgoingCrossNode const & startNode,
                                                                TRoutingMappingPtr const & currentMapping) const
{
  // Check cached crosses.
  auto const key = make_pair(startNode.m_nodeId, currentMapping->GetMwmId());
  auto const it = m_cachedNextNodes.find(key);
  if (it != m_cachedNextNodes.end())
    return it->second;

  // Cache miss case.
  auto & crosses = m_cachedNextNodes[key];
  ConstructBorderCrossImpl(startNode, currentMapping, crosses);
  return crosses;
}

bool CrossMwmGraph::ConstructBorderCrossImpl(OutgoingCrossNode const & startNode,
                                             TRoutingMappingPtr const & currentMapping,
                                             vector<BorderCross> & crosses) const
{
  string const & nextMwm = currentMapping->m_crossContext.GetOutgoingMwmName(startNode);
  TRoutingMappingPtr nextMapping = m_indexManager.GetMappingByName(nextMwm);
  // If we haven't this routing file, we skip this path.
  if (!nextMapping->IsValid())
    return false;
  crosses.clear();
  nextMapping->LoadCrossContext();
  nextMapping->m_crossContext.ForEachIngoingNodeNearPoint(startNode.m_point, [&](IngoingCrossNode const & node)
  {
      crosses.emplace_back(CrossNode(startNode.m_nodeId, currentMapping->GetMwmId(), node.m_point),
                           CrossNode(node.m_nodeId, nextMapping->GetMwmId(), node.m_point));
  });
  return !crosses.empty();
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
  TRoutingMappingPtr currentMapping = m_indexManager.GetMappingById(v.toNode.mwmId);
  ASSERT(currentMapping->IsValid(), ());
  currentMapping->LoadCrossContext();
  currentMapping->FreeFileIfPossible();

  CrossRoutingContextReader const & currentContext = currentMapping->m_crossContext;

  // Find income node.
  IngoingCrossNode ingoingNode;
  bool found = false;
  auto findingFn = [&ingoingNode, &v, &found](IngoingCrossNode const & node)
                                             {
                                               if (node.m_nodeId == v.toNode.node)
                                               {
                                                 found = true;
                                                 ingoingNode = node;
                                               }
                                             };
  CHECK(currentContext.ForEachIngoingNodeNearPoint(v.toNode.point, findingFn), ());

  CHECK(found, ());

  // Find outs. Generate adjacency list.
  currentContext.ForEachOutgoingNode([&, this](OutgoingCrossNode const & node)
                                     {
                                       EdgeWeight const outWeight = currentContext.GetAdjacencyCost(ingoingNode, node);
                                       if (outWeight != kInvalidContextEdgeWeight && outWeight != 0)
                                       {
                                         vector<BorderCross> const & targets = ConstructBorderCross(node, currentMapping);
                                         for (auto const & target : targets)
                                         {
                                           if (target.toNode.IsValid())
                                             adj.emplace_back(target, outWeight);
                                         }
                                       }
                                     });
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
    ASSERT_EQUAL(graphCrosses[i].toNode.mwmId, graphCrosses[i + 1].fromNode.mwmId, ());
    route.emplace_back(graphCrosses[i].toNode.node,
                       MercatorBounds::FromLatLon(graphCrosses[i].toNode.point),
                       graphCrosses[i + 1].fromNode.node,
                       MercatorBounds::FromLatLon(graphCrosses[i + 1].fromNode.point),
                       graphCrosses[i].toNode.mwmId);
  }

  if (route.empty())
    return;

  route.front().startNode = startGraphNode;
  route.back().finalNode = finalGraphNode;
  ASSERT_EQUAL(route.front().startNode.mwmId, route.front().finalNode.mwmId, ());
  ASSERT_EQUAL(route.back().startNode.mwmId, route.back().finalNode.mwmId, ());
}

}  // namespace routing
