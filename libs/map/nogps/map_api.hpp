#pragma once

#include "map/nogps/delegate.hpp"
#include "map/nogps/road_walker.hpp"
#include "map/nogps/roads.hpp"

#include "geometry/latlon.hpp"

#include <optional>
#include <string>

namespace nogps
{
/// The roads, the followed route and the map the navigation without GPS works with.
class MapApi
{
public:
  virtual ~MapApi() = default;

  virtual bool IsNavigating() const = 0;

  /// \param matchRoute prefer the followed route to the roads around, it is where the car really is.
  virtual Roads & GetRoads(bool matchRoute) = 0;

  /// Rebuilds the route at once if the position is off the route, without waiting for the several consecutive
  /// off-route positions as for GPS. The route goes the |bearingDeg| way if it is known.
  virtual void RebuildRouteIfOffRoute(Fix const & fix, double offRouteDistanceM, std::optional<double> bearingDeg) = 0;

  /// \returns the closest point of the followed route within |radiusM| and the direction of the route there.
  virtual std::optional<RoadPoint> ProjectToRoute(ms::LatLon const & position, double radiusM) = 0;

  /// Moves the position along the followed route, stopping at the closest turn or crossing in both directions.
  /// \param bearingDeg where the car looks.
  /// \param distanceM meters to move forward, negative to move back.
  /// \returns nothing if there is no followed route, the position is not on it or it is moved back from the start of
  /// the route.
  virtual std::optional<RoadWalker::Result> ShiftAlongRoute(ms::LatLon const & position,
                                                            std::optional<double> bearingDeg, double distanceM) = 0;

  /// \returns the closest road within |radiusM|, a main road rather than a driveway branching off it nearly as
  /// close.
  virtual std::optional<RoadPoint> SnapToMainRoad(ms::LatLon const & position, double radiusM) = 0;

  /// \returns the distance left to the end of the route for the log of a drive, empty if there is no route.
  virtual std::string GetDistanceLeft() const = 0;

  /// Shows the car direction instead of the compass: without GPS the car looks along its road, and the compass of
  /// a phone in a car or in hands looks anywhere.
  virtual void ShowCarHeading(double bearingDeg) = 0;

  /// Shows where the car is by the calculation while it maneuvers off the roads and its mark waits on the road
  /// of the followed route. Nothing hides it.
  virtual void ShowOffRoadCar(std::optional<ms::LatLon> const & position) = 0;
};
}  // namespace nogps
