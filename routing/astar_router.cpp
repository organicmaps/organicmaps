#include "routing/astar_router.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"
#include "std/algorithm.hpp"

namespace routing
{
IRouter::ResultCode AStarRouter::CalculateRouteM2M(vector<RoadPos> const & startPos,
                                                   vector<RoadPos> const & finalPos,
                                                   vector<RoadPos> & route)
{
  RoadGraph graph(*m_roadGraph);
  m_algo.SetGraph(graph);

  // TODO (@gorshenin): switch to FindPathBidirectional.
  Algo::Result result = m_algo.FindPath(startPos, finalPos, route);
  switch (result)
  {
    case Algo::RESULT_OK:
      // Following hack is used because operator== checks for
      // equivalience, not identity, since it doesn't test
      // RoadPos::m_segEndpoint. Thus, start and final positions
      // returned by algorithm should be replaced by correct start and
      // final positions.
      for (RoadPos const & sp : startPos)
      {
        if (route.front() == sp)
          route.front() = sp;
      }
      for (RoadPos const & fp : finalPos)
      {
        if (route.back() == fp)
          route.back() = fp;
      }
      return IRouter::NoError;
    case Algo::RESULT_NO_PATH:
      return IRouter::RouteNotFound;
    case Algo::RESULT_CANCELLED:
      return IRouter::Cancelled;
  }
}
}  // namespace routing
