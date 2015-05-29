#include "cross_mwm_router.hpp"
#include "cross_mwm_road_graph.hpp"
#include "base/astar_algorithm.hpp"

namespace routing
{
using TAlgorithm = AStarAlgorithm<CrossMwmGraph>;

/// Function to run AStar Algorithm from the base.
IRouter::ResultCode CalculateRoute(TCrossPair const & startPos, TCrossPair const & finalPos,
                                   CrossMwmGraph const & roadGraph, vector<TCrossPair> & route,
                                   RoutingVisualizerFn const & routingVisualizer)
{
  TAlgorithm m_algo;
  m_algo.SetGraph(roadGraph);

  TAlgorithm::OnVisitedVertexCallback onVisitedVertex = nullptr;
  if (routingVisualizer)
    onVisitedVertex = [&routingVisualizer](TCrossPair const & cross)
    {
      routingVisualizer(cross.first.point);
    };

  TAlgorithm::Result const result =m_algo.FindPath(startPos, finalPos, route, onVisitedVertex);
  switch (result)
  {
    case TAlgorithm::Result::OK:
      ASSERT_EQUAL(route.front(), startPos, ());
      ASSERT_EQUAL(route.back(), finalPos, ());
      return IRouter::NoError;
    case TAlgorithm::Result::NoPath:
      return IRouter::RouteNotFound;
    case TAlgorithm::Result::Cancelled:
      return IRouter::Cancelled;
  }
  return IRouter::RouteNotFound;
}

IRouter::ResultCode CalculateCrossMwmPath(TRoutingNodes const & startGraphNodes,
                                          TRoutingNodes const & finalGraphNodes,
                                          RoutingIndexManager & indexManager,
                                          RoutingVisualizerFn const & routingVisualizer,
                                          TCheckedPath & route)
{
  CrossMwmGraph roadGraph(indexManager);
  FeatureGraphNode startGraphNode, finalGraphNode;
  CrossNode startNode, finalNode;
  IRouter::ResultCode code;

  // Finding start node.
  for (FeatureGraphNode const & start : startGraphNodes)
  {
    startNode = CrossNode(start.node.forward_node_id, start.mwmName, start.segmentPoint);
    code = roadGraph.SetStartNode(startNode);
    if (code == IRouter::NoError)
    {
      startGraphNode = start;
      break;
    }
  }
  if (code != IRouter::NoError)
    return IRouter::StartPointNotFound;

  // Finding final node.
  for (FeatureGraphNode const & final : finalGraphNodes)
  {
    finalNode = CrossNode(final.node.reverse_node_id, final.mwmName, final.segmentPoint);
    code = roadGraph.SetFinalNode(finalNode);
    if (code == IRouter::NoError)
    {
      finalGraphNode = final;
      break;
    }
  }
  if (code != IRouter::NoError)
    return IRouter::EndPointNotFound;

  // Finding path through maps.
  vector<TCrossPair> tempRoad;
  code = CalculateRoute({startNode, startNode}, {finalNode, finalNode}, roadGraph, tempRoad,
                        routingVisualizer);
  if (code != IRouter::NoError)
    return code;

  // Final path conversion to output type.
  for (size_t i = 0; i < tempRoad.size() - 1; ++i)
  {
    route.emplace_back(tempRoad[i].second.node, tempRoad[i + 1].first.node,
                       tempRoad[i].second.mwmName);
  }

  if (!route.empty())
  {
    route.front().startNode = startGraphNode;
    route.back().finalNode = finalGraphNode;
    return IRouter::NoError;
  }
  return IRouter::RouteNotFound;
}

}  // namespace routing
