#include "routing/astar_router.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"
#include "std/algorithm.hpp"

namespace routing
{
AStarRouter::AStarRouter(CountryFileFnT const & fn, Index const * pIndex,
                         RoutingVisualizerFn routingVisualizer)
    : RoadGraphRouter(pIndex, unique_ptr<IVehicleModel>(new PedestrianModel()), fn),
      m_routingVisualizer(routingVisualizer)
{
}

IRouter::ResultCode AStarRouter::CalculateRoute(RoadPos const & startPos, RoadPos const & finalPos,
                                                vector<RoadPos> & route)
{
  RoadGraph graph(*m_roadGraph);
  m_algo.SetGraph(graph);

  TAlgorithm::OnVisitedVertexCallback onVisitedVertexCallback = nullptr;
  if (nullptr != m_routingVisualizer)
    onVisitedVertexCallback = [this](RoadPos const & roadPos) { m_routingVisualizer(roadPos.GetSegEndpoint()); };

  TAlgorithm::Result const result = m_algo.FindPathBidirectional(startPos, finalPos, route, onVisitedVertexCallback);
  switch (result)
  {
    case TAlgorithm::Result::OK:
      // Following hack is used because operator== checks for
      // equivalience, not identity, since it doesn't test
      // RoadPos::m_segEndpoint. Thus, start and final positions
      // returned by algorithm should be replaced by correct start and
      // final positions.
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
