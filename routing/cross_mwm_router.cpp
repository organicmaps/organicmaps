#include "routing/cross_mwm_router.hpp"

#include "routing/base/astar_algorithm.hpp"
#include "routing/base/routing_result.hpp"
#include "routing/cross_mwm_road_graph.hpp"

#include "base/timer.hpp"

namespace routing
{

namespace
{
/// Function to run AStar Algorithm from the base.
IRouter::ResultCode CalculateRoute(BorderCross const & startPos, BorderCross const & finalPos,
                                   CrossMwmRoadGraph & roadGraph, RouterDelegate const & delegate,
                                   RoutingResult<BorderCross, double /* Weight */> & route)
{
  using Algorithm = AStarAlgorithm<CrossMwmRoadGraph>;

  Algorithm::OnVisitedVertexCallback onVisitedVertex =
      [&delegate](BorderCross const & cross, BorderCross const & /* target */) {
        delegate.OnPointCheck(MercatorBounds::FromLatLon(cross.fromNode.point));
      };

  Algorithm::Params params(roadGraph, startPos, finalPos, nullptr /* prevRoute */, delegate,
                           onVisitedVertex, {} /* checkLengthCallback */);

  my::HighResTimer timer(true);
  Algorithm::Result const result = Algorithm().FindPath(params, route);
  LOG(LINFO, ("Duration of the cross MWM path finding", timer.ElapsedNano()));
  switch (result)
  {
  case Algorithm::Result::OK:
    ASSERT_EQUAL(route.m_path.front(), startPos, ());
    ASSERT_EQUAL(route.m_path.back(), finalPos, ());
    return IRouter::NoError;
  case Algorithm::Result::NoPath: return IRouter::RouteNotFound;
  case Algorithm::Result::Cancelled: return IRouter::Cancelled;
  }
  return IRouter::RouteNotFound;
}
}  // namespace

IRouter::ResultCode CalculateCrossMwmPath(TRoutingNodes const & startGraphNodes,
                                          TRoutingNodes const & finalGraphNodes,
                                          RoutingIndexManager & indexManager,
                                          double & cost,
                                          RouterDelegate const & delegate, TCheckedPath & route)
{
  CrossMwmRoadGraph roadGraph(indexManager);
  FeatureGraphNode startGraphNode, finalGraphNode;
  CrossNode startNode, finalNode;

  // Finding start node.
  IRouter::ResultCode code = IRouter::StartPointNotFound;
  for (FeatureGraphNode const & start : startGraphNodes)
  {
    startNode = CrossNode(start.node.forward_node_id, start.node.reverse_node_id, start.mwmId,
                          MercatorBounds::ToLatLon(start.segmentPoint));
    code = roadGraph.SetStartNode(startNode);
    if (code == IRouter::NoError)
    {
      startGraphNode = start;
      break;
    }
    if (delegate.IsCancelled())
      return IRouter::Cancelled;
  }
  if (code != IRouter::NoError)
    return IRouter::StartPointNotFound;

  // Finding final node.
  code = IRouter::EndPointNotFound;
  for (FeatureGraphNode const & final : finalGraphNodes)
  {
    finalNode = CrossNode(final.node.reverse_node_id, final.node.forward_node_id, final.mwmId,
                          MercatorBounds::ToLatLon(final.segmentPoint));
    finalNode.isVirtual = true;
    code = roadGraph.SetFinalNode(finalNode);
    if (code == IRouter::NoError)
    {
      finalGraphNode = final;
      break;
    }
    if (delegate.IsCancelled())
      return IRouter::Cancelled;
  }
  if (code != IRouter::NoError)
    return IRouter::EndPointNotFound;

  // Finding path through maps.
  RoutingResult<BorderCross, double /* Weight */> tempRoad;
  code = CalculateRoute({startNode, startNode}, {finalNode, finalNode}, roadGraph, delegate, tempRoad);
  cost = tempRoad.m_distance;
  if (code != IRouter::NoError)
    return code;
  if (delegate.IsCancelled())
    return IRouter::Cancelled;

  // Final path conversion to output type.
  ConvertToSingleRouterTasks(tempRoad.m_path, startGraphNode, finalGraphNode, route);

  return route.empty() ? IRouter::RouteNotFound : IRouter::NoError;
}

}  // namespace routing
