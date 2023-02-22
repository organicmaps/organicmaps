#include "routing/helicopter_router.hpp"
#include "routing/route.hpp"

namespace routing
{
using namespace std;

void HelicopterRouter::ClearState()
{
}

void HelicopterRouter::SetGuides(GuidesTracks && guides) { /*m_guides = GuidesConnections(guides);*/ }

RouterResultCode HelicopterRouter::CalculateRoute(Checkpoints const & checkpoints,
                                             m2::PointD const & startDirection,
                                             bool adjustToPrevRoute,
                                             RouterDelegate const & delegate, Route & route)
{
  vector<m2::PointD> points = checkpoints.GetPoints();
  geometry::Altitude const mockAltitude = 0;

  vector<RouteSegment> routeSegments;
  for(size_t i=1; i<points.size(); i++) {
    Segment segment(kFakeNumMwmId, 0, 0, false);
    turns::TurnItem turn;
    turn.m_index = i;
    if (i == points.size()-1)
      turn.m_pedestrianTurn = turns::PedestrianDirection::ReachedYourDestination;
    geometry::PointWithAltitude junction(points[i], mockAltitude);
    RouteSegment::RoadNameInfo roadNameInfo;

    auto routeSegment = RouteSegment(segment, turn, junction, roadNameInfo);
    routeSegments.push_back(move(routeSegment));
  }
  route.SetRouteSegments(move(routeSegments));

  vector<Route::SubrouteAttrs> subroutes;
  for(size_t i=1; i<points.size(); i++) {
    auto subrt = Route::SubrouteAttrs(geometry::PointWithAltitude(points[i-1], mockAltitude),
                                      geometry::PointWithAltitude(points[i], mockAltitude), i-1, i);
    subroutes.push_back(move(subrt));
  }
  route.SetCurrentSubrouteIdx(checkpoints.GetPassedIdx());
  route.SetSubroteAttrs(move(subroutes));

  vector<m2::PointD> routeGeometry;
  routeGeometry.insert(end(routeGeometry),
                       begin(points),
                       end(points));

  route.SetGeometry(routeGeometry.begin(), routeGeometry.end());

  return RouterResultCode::NoError;
}

bool HelicopterRouter::FindClosestProjectionToRoad(m2::PointD const & point,
                                              m2::PointD const & direction, double radius,
                                              EdgeProj & proj)
{
  //TODO
  return false;
}

}// namespace routing
