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
                                   RoutingResult<BorderCross, double /* WeightType */> & route)
{
  using TAlgorithm = AStarAlgorithm<CrossMwmRoadGraph>;

  TAlgorithm::TOnVisitedVertexCallback onVisitedVertex =
      [&delegate](BorderCross const & cross, BorderCross const & /* target */)
  {
    delegate.OnPointCheck(MercatorBounds::FromLatLon(cross.fromNode.point));
  };

  my::HighResTimer timer(true);
  TAlgorithm::Result const result =
      TAlgorithm().FindPath(roadGraph, startPos, finalPos, route, delegate, onVisitedVertex);
  LOG(LINFO, ("Duration of the cross MWM path finding", timer.ElapsedNano()));
  switch (result)
  {
    case TAlgorithm::Result::OK:
      ASSERT_EQUAL(route.path.front(), startPos, ());
      ASSERT_EQUAL(route.path.back(), finalPos, ());
      return IRouter::NoError;
    case TAlgorithm::Result::NoPath:
      return IRouter::RouteNotFound;
    case TAlgorithm::Result::Cancelled:
      return IRouter::Cancelled;
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
  RoutingResult<BorderCross, double /* WeightType */> tempRoad;
  code = CalculateRoute({startNode, startNode}, {finalNode, finalNode}, roadGraph, delegate, tempRoad);
  cost = tempRoad.distance;
  if (code != IRouter::NoError)
    return code;
  if (delegate.IsCancelled())
    return IRouter::Cancelled;

  // Final path conversion to output type.
  ConvertToSingleRouterTasks(tempRoad.path, startGraphNode, finalGraphNode, route);

  return route.empty() ? IRouter::RouteNotFound : IRouter::NoError;
}

}  // namespace routing
