#include "routing/astar_router.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"
#include "std/algorithm.hpp"

namespace routing
{
IRouter::ResultCode AStarRouter::CalculateRoute(RoadPos const & startPos, RoadPos const & finalPos,
                                                vector<RoadPos> & route)
{
  RoadGraph graph(*m_roadGraph);
  m_algo.SetGraph(graph);

  TAlgorithm::Result result = m_algo.FindPathBidirectional(startPos, finalPos, route);
  switch (result)
  {
    case TAlgorithm::Result::OK:
      // Following hack is used because operator== checks for
      // equivalience, not identity, since it doesn't test
      // RoadPos::m_segEndpoint. Thus, start and final positions
      // returned by algorithm should be replaced by correct start and
      // final positions.
      ASSERT_EQUAL(route.front(), startPos, ());
      route.front() = startPos;
      ASSERT_EQUAL(route.back(), finalPos, ());
      route.back() = finalPos;
      return IRouter::NoError;
    case TAlgorithm::Result::NoPath:
      return IRouter::RouteNotFound;
    case TAlgorithm::Result::Cancelled:
      return IRouter::Cancelled;
  }
}
}  // namespace routing
