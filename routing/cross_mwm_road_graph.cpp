#include "cross_mwm_road_graph.hpp"
#include "cross_mwm_router.hpp"

#include "geometry/distance_on_sphere.hpp"

namespace
{
inline bool IsValidEdgeWeight(EdgeWeight const & w) { return w != INVALID_EDGE_WEIGHT; }

double constexpr kMwmCrossingNodeEqualityRadiusMeters = 5.0;
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
  auto const mwmOutsIter = startMapping->m_crossContext.GetOutgoingIterators();
  size_t const outSize = distance(mwmOutsIter.first, mwmOutsIter.second);
  // Can't find the route if there are no routes outside source map.
  if (!outSize)
    return IRouter::RouteNotFound;

  // Generate routing task from one source to several targets.
  TRoutingNodes sources(1), targets;
  targets.reserve(outSize);
  for (auto j = mwmOutsIter.first; j < mwmOutsIter.second; ++j)
    targets.emplace_back(j->m_nodeId, false /* isStartNode */, startNode.mwmName);
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
      BorderCross nextNode = FindNextMwmNode(*(mwmOutsIter.first + i), startMapping);
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
  CrossNode start(node.m_nodeId, finalNode.mwmName,
                  MercatorBounds::FromLatLon(node.m_point.y, node.m_point.x));
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
  auto const mwmIngoingIter = finalMapping->m_crossContext.GetIngoingIterators();
  // Generate routing task from one source to several targets.
  TRoutingNodes sources, targets(1);
  size_t const ingoingSize = distance(mwmIngoingIter.first, mwmIngoingIter.second);
  sources.reserve(ingoingSize);

  // If there is no routes inside target map.
  if (!ingoingSize)
    return IRouter::RouteNotFound;

  for (auto j = mwmIngoingIter.first; j != mwmIngoingIter.second; ++j)
  {
    // Case with a target node at the income mwm node.
    if (j->m_nodeId == finalNode.node)
    {
      AddVirtualEdge(*j, finalNode, 0 /* no weight */);
      return IRouter::NoError;
    }
    sources.emplace_back(j->m_nodeId, true /* isStartNode */, finalNode.mwmName);
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
      AddVirtualEdge(*(mwmIngoingIter.first + i), finalNode, weights[i]);
    }
  }
  return IRouter::NoError;
}

BorderCross CrossMwmGraph::FindNextMwmNode(OutgoingCrossNode const & startNode,
                                           TRoutingMappingPtr const & currentMapping) const
{
  m2::PointD const & startPoint = startNode.m_point;

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

  auto incomeIters = nextMapping->m_crossContext.GetIngoingIterators();
  for (auto i = incomeIters.first; i < incomeIters.second; ++i)
  {
    m2::PointD const & targetPoint = i->m_point;
    if (ms::DistanceOnEarth(startPoint.y, startPoint.x, targetPoint.y, targetPoint.x) <
        kMwmCrossingNodeEqualityRadiusMeters)
    {
      BorderCross const cross(
          CrossNode(startNode.m_nodeId, currentMapping->GetCountryName(),
                    MercatorBounds::FromLatLon(targetPoint.y, targetPoint.x)),
          CrossNode(i->m_nodeId, nextMwm,
                    MercatorBounds::FromLatLon(targetPoint.y, targetPoint.x)));
      m_cachedNextNodes.insert(make_pair(startPoint, cross));
      return cross;
    }
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
  auto inRange = currentContext.GetIngoingIterators();
  auto outRange = currentContext.GetOutgoingIterators();

  // Find income node.
  auto inIt = inRange.first;
  while (inIt != inRange.second)
  {
    if (inIt->m_nodeId == v.toNode.node)
      break;
    ++inIt;
  }
  CHECK(inIt != inRange.second, ());
  // Find outs. Generate adjacency list.
  for (auto outIt = outRange.first; outIt != outRange.second; ++outIt)
  {
    EdgeWeight const outWeight = currentContext.GetAdjacencyCost(inIt, outIt);
    if (outWeight != INVALID_CONTEXT_EDGE_WEIGHT && outWeight != 0)
    {
      BorderCross target = FindNextMwmNode(*outIt, currentMapping);
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
    route.emplace_back(graphCrosses[i].toNode.node, graphCrosses[i + 1].fromNode.node,
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
