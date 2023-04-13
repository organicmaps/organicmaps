#include "routing/helicopter_router.hpp"
#include "routing/route.hpp"
#include "routing/routing_helpers.hpp"

namespace routing
{
using namespace std;

void HelicopterRouter::ClearState()
{
}

void HelicopterRouter::SetGuides(GuidesTracks && guides) { /*m_guides = GuidesConnections(guides);*/ }


/* Helicopter router doesn't read roads graph and uses only checkpoints to build a route.

      1-----------2
     /             \
    /               \
   /                 F
  S

  For example we need to build route from start (S) to finish (F) with intermidiate
  points (1) and (2).
  Target route should have parameters:

  m_geometry      = [S, S, 1, 1, 2, 2, F, F]
  m_routeSegments = [S, 1, 1, 2, 2, F, F]
  m_subroutes     =
    +-------+--------+-----------------+---------------+
    | start | finish | beginSegmentIdx | endSegmentIdx |
    +-------+--------+-----------------+---------------+
    | S     | 1      | 0               | 1             |
    | 1     | 1      | 1               | 2             |
    | 1     | 2      | 2               | 3             |
    | 2     | 2      | 3               | 4             |
    | 2     | F      | 4               | 6             |
    | 2     | F      | 4               | 6             |
    +-------+--------+-----------------+---------------+

  Constraints:
  *  m_geometry.size() == 2 * checkpoints.size()
  *  m_routeSegments.size() == m_geometry.size()-1
  *  m_subroutes.size() == m_routeSegments.size()-1

 */
RouterResultCode HelicopterRouter::CalculateRoute(Checkpoints const & checkpoints,
                                             m2::PointD const & startDirection,
                                             bool adjustToPrevRoute,
                                             RouterDelegate const & delegate, Route & route)
{
  vector<m2::PointD> const & points = checkpoints.GetPoints();
  geometry::Altitude const mockAltitude = 0;
  vector<RouteSegment> routeSegments;
  vector<double> times;
  times.reserve(points.size()*2-1);
  Segment const segment(kFakeNumMwmId, 0, 0, false);

  for (size_t i = 0; i < points.size(); ++i)
  {
    turns::TurnItem turn(i, turns::PedestrianDirection::None);
    geometry::PointWithAltitude const junction(points[i], mockAltitude);
    RouteSegment::RoadNameInfo const roadNameInfo;

    auto routeSegment = RouteSegment(segment, turn, junction, roadNameInfo);
    routeSegments.push_back(move(routeSegment));
    times.push_back(0);

    if (i == points.size() - 1)
    {
      // Create final segment.
      turn = turns::TurnItem(i+1, turns::PedestrianDirection::ReachedYourDestination);
      RouteSegment lastSegment = RouteSegment(segment, turn, junction, roadNameInfo);
      routeSegments.push_back(move(lastSegment));
      times.push_back(0);
    }
    else if (i > 0)
    {
      // Duplicate intermidiate points.
      RouteSegment intermidiateSegment = RouteSegment(segment, turn, junction, roadNameInfo);
      routeSegments.push_back(move(intermidiateSegment));
      times.push_back(0);
    }
  }

  FillSegmentInfo(times, routeSegments);
  route.SetRouteSegments(move(routeSegments));

  vector<Route::SubrouteAttrs> subroutes;
  for(size_t i = 1; i < points.size(); ++i)
  {
    if (i<points.size()-1)
    {
      auto subrt1 = Route::SubrouteAttrs(geometry::PointWithAltitude(points[i-1], mockAltitude),
                                        geometry::PointWithAltitude(points[i], mockAltitude), i*2-2, i*2-1);
      subroutes.push_back(move(subrt1));

      auto subrt2 = Route::SubrouteAttrs(geometry::PointWithAltitude(points[i-1], mockAltitude),
                                        geometry::PointWithAltitude(points[i], mockAltitude), i*2-1, i*2);
      subroutes.push_back(move(subrt2));
    }
    else
    {
        // Duplicate last subroute attrs.
        auto subrt = Route::SubrouteAttrs(geometry::PointWithAltitude(points[i-1], mockAltitude),
                                          geometry::PointWithAltitude(points[i+1], mockAltitude), i*2-2, i*2);
        subroutes.push_back(move(subrt));

        subrt = Route::SubrouteAttrs(geometry::PointWithAltitude(points[i-1], mockAltitude),
                                     geometry::PointWithAltitude(points[i+1], mockAltitude), i*2-2, i*2);
        subroutes.push_back(move(subrt));
    }
  }

  route.SetCurrentSubrouteIdx(checkpoints.GetPassedIdx());
  route.SetSubroteAttrs(move(subroutes));

  vector<m2::PointD> routeGeometry;
  for (auto p: points)
  {
    routeGeometry.push_back(p);
    routeGeometry.push_back(p);
  }

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

}  // namespace routing
