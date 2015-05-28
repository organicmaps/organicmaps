#include "cross_mwm_road_graph.hpp"

#include "geometry/distance_on_sphere.hpp"

namespace
{
inline bool IsValidEdgeWeight(EdgeWeight const & w) { return w != INVALID_EDGE_WEIGHT; }
double const kMwmCrossingNodeEqualityRadiusMeters = 1000.0;
}

namespace routing
{
IRouter::ResultCode CrossMwmGraph::SetStartNode(CrossNode const & startNode)
{
  ASSERT(startNode.mwmName.length(), ());
  // TODO make cancellation
  TRoutingMappingPtr startMapping = m_indexManager.GetMappingByName(startNode.mwmName);
  MappingGuard startMappingGuard(startMapping);
  UNUSED_VALUE(startMappingGuard);
  startMapping->LoadCrossContext();

  // Load source data
  auto const mwmOutsIter = startMapping->m_crossContext.GetOutgoingIterators();
  // Generate routing task from one source to several targets
  TRoutingNodes sources(1), targets;
  size_t const outSize = distance(mwmOutsIter.first, mwmOutsIter.second);
  targets.reserve(outSize);

  // If there is no routes outside source map
  if (!outSize)
    return IRouter::RouteNotFound;

  for (auto j = mwmOutsIter.first; j < mwmOutsIter.second; ++j)
    targets.emplace_back(j->m_nodeId, false, startNode.mwmName);
  vector<EdgeWeight> weights;

  sources[0] = FeatureGraphNode(startNode.node, true, startNode.mwmName);
  FindWeightsMatrix(sources, targets, startMapping->m_dataFacade, weights);
  if (find_if(weights.begin(), weights.end(), &IsValidEdgeWeight) == weights.end())
    return IRouter::StartPointNotFound;
  vector<CrossWeightedEdge> dummyEdges;
  for (size_t i = 0; i < outSize; ++i)
  {
    if (IsValidEdgeWeight(weights[i]))
    {
      TCrossPair nextNode = FindNextMwmNode(*(mwmOutsIter.first + i), startMapping);
      if (nextNode.second.IsValid())
        dummyEdges.emplace_back(nextNode, weights[i]);
    }
  }

  m_virtualEdges.insert(make_pair(startNode, dummyEdges));
  return IRouter::NoError;
}

IRouter::ResultCode CrossMwmGraph::SetFinalNode(CrossNode const & finalNode)
{
  ASSERT(finalNode.mwmName.length(), ());
  TRoutingMappingPtr finalMapping = m_indexManager.GetMappingByName(finalNode.mwmName);
  MappingGuard finalMappingGuard(finalMapping);
  UNUSED_VALUE(finalMappingGuard);
  finalMapping->LoadCrossContext();

  // Load source data
  auto const mwmIngoingIter = finalMapping->m_crossContext.GetIngoingIterators();
  // Generate routing task from one source to several targets
  TRoutingNodes sources, targets(1);
  size_t const ingoingSize = distance(mwmIngoingIter.first, mwmIngoingIter.second);
  sources.reserve(ingoingSize);

  // If there is no routes inside target map
  if (!ingoingSize)
    return IRouter::RouteNotFound;

  for (auto j = mwmIngoingIter.first; j < mwmIngoingIter.second; ++j)
    sources.emplace_back(j->m_nodeId, true, finalNode.mwmName);
  vector<EdgeWeight> weights;

  targets[0] = FeatureGraphNode(finalNode.node, false, finalNode.mwmName);
  FindWeightsMatrix(sources, targets, finalMapping->m_dataFacade, weights);
  if (find_if(weights.begin(), weights.end(), &IsValidEdgeWeight) == weights.end())
    return IRouter::StartPointNotFound;
  for (size_t i = 0; i < ingoingSize; ++i)
  {
    if (IsValidEdgeWeight(weights[i]))
    {
      IngoingCrossNode const & iNode = *(mwmIngoingIter.first + i);
      CrossNode start(iNode.m_nodeId, finalNode.mwmName,
                      MercatorBounds::FromLatLon(iNode.m_point.y, iNode.m_point.x));
      vector<CrossWeightedEdge> dummyEdges;
      dummyEdges.emplace_back(make_pair(finalNode, finalNode), weights[i]);
      m_virtualEdges.insert(make_pair(start, dummyEdges));
    }
  }
  return IRouter::NoError;
}

TCrossPair CrossMwmGraph::FindNextMwmNode(OutgoingCrossNode const & startNode,
                                          TRoutingMappingPtr const & currentMapping) const
{
  m2::PointD const & startPoint = startNode.m_point;

  string const & nextMwm = currentMapping->m_crossContext.getOutgoingMwmName(startNode);
  TRoutingMappingPtr nextMapping;
  nextMapping = m_indexManager.GetMappingByName(nextMwm);
  // If we haven't this routing file, we skip this path
  if (!nextMapping->IsValid())
    return TCrossPair();
  nextMapping->LoadCrossContext();

  auto income_iters = nextMapping->m_crossContext.GetIngoingIterators();
  for (auto i = income_iters.first; i < income_iters.second; ++i)
  {
    m2::PointD const & targetPoint = i->m_point;
    if (ms::DistanceOnEarth(startPoint.y, startPoint.x, targetPoint.y, targetPoint.x) <
        kMwmCrossingNodeEqualityRadiusMeters)
      return make_pair(CrossNode(startNode.m_nodeId, currentMapping->GetName(),
                                 MercatorBounds::FromLatLon(targetPoint.y, targetPoint.x)),
                       CrossNode(i->m_nodeId, nextMwm,
                                 MercatorBounds::FromLatLon(targetPoint.y, targetPoint.x)));
  }
  return TCrossPair();
}

void CrossMwmGraph::GetOutgoingEdgesListImpl(TCrossPair const & v,
                                             vector<CrossWeightedEdge> & adj) const
{
  // Check for virtual edges
  adj.clear();
  auto const it = m_virtualEdges.find(v.second);
  if (it != m_virtualEdges.end())
  {
    for (auto const & a : it->second)
    {
      ASSERT(a.GetTarget().first.IsValid(), ());
      adj.push_back(a);
    }
    return;
  }

  // Loading cross roating section.
  TRoutingMappingPtr currentMapping = m_indexManager.GetMappingByName(v.second.mwmName);
  ASSERT(currentMapping->IsValid(), ());
  currentMapping->LoadCrossContext();

  CrossRoutingContextReader const & currentContext = currentMapping->m_crossContext;
  auto current_in_iterators = currentContext.GetIngoingIterators();
  auto current_out_iterators = currentContext.GetOutgoingIterators();

  // Find income node.
  auto iit = current_in_iterators.first;
  while (iit < current_in_iterators.second)
  {
    if (iit->m_nodeId == v.second.node)
      break;
    ++iit;
  }
  ASSERT(iit != current_in_iterators.second, ());
  // Find outs. Generate adjacency list.
  for (auto oit = current_out_iterators.first; oit != current_out_iterators.second; ++oit)
  {
    EdgeWeight const outWeight = currentContext.getAdjacencyCost(iit, oit);
    if (outWeight != INVALID_CONTEXT_EDGE_WEIGHT && outWeight != 0)
    {
      TCrossPair target = FindNextMwmNode(*oit, currentMapping);
      if (target.second.IsValid())
        adj.emplace_back(target, outWeight);
    }
  }
}

}  // namespace routing
