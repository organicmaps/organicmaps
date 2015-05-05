#include "routing/astar_router.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"
#include "std/algorithm.hpp"

namespace routing
{

AStarRouter::AStarRouter(TMwmFileByPointFn const & fn, Index const * pIndex, RoutingVisualizerFn routingVisualizer)
    : RoadGraphRouter(pIndex, unique_ptr<IVehicleModel>(new PedestrianModel()), fn)
    , m_routingVisualizer(routingVisualizer)
{
}

IRouter::ResultCode AStarRouter::CalculateRoute(Junction const & startPos, Junction const & finalPos,
                                                vector<Junction> & route)
{
  RoadGraph const roadGraph(*GetGraph());
  m_algo.SetGraph(roadGraph);

  TAlgorithm::OnVisitedVertexCallback onVisitedVertexCallback = nullptr;
  if (nullptr != m_routingVisualizer)
    onVisitedVertexCallback = [this](Junction const & junction) { m_routingVisualizer(junction.GetPoint()); };

  TAlgorithm::Result const result = m_algo.FindPathBidirectional(startPos, finalPos, route, onVisitedVertexCallback);
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

}  // namespace routing
