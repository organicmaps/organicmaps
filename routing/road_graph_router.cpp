#include "road_graph_router.hpp"
#include "route.hpp"

namespace routing
{

void RoadGraphRouter::SetFinalPoint(m2::PointD const & finalPt)
{
  // TODO: Calculate finalPos.
  vector<RoadPos> finalPos;
  this->SetFinalRoadPos(finalPos);
}

void RoadGraphRouter::CalculateRoute(m2::PointD const & startPt, ReadyCallback const & callback)
{
  // TODO: Calculate startPos.
  vector<RoadPos> startPos;
  vector<RoadPos> route;
  this->CalculateRoute(startPos, route);
  // TODO: Call back the callback
}

} // namespace routing

