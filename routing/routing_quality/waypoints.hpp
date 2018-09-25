#pragma once

#include "routing/routing_quality/utils.hpp"
#include "routing/vehicle_mask.hpp"

#include "geometry/latlon.hpp"

#include <vector>

namespace routing_quality
{
struct RouteParams;

struct ReferenceRoute
{
  /// \brief Waypoints which the route passes through.
  Coordinates m_waypoints;
  /// \brief Value in range (0.0; 1.0] which indicates how desirable the route is.
  double m_factor = 1.0;
};

/// \brief There can be more than one reference route.
using ReferenceRoutes = std::vector<ReferenceRoute>;

using Similarity = double;

/// \brief Checks how many reference waypoints the route contains.
/// \returns normalized value in range [0.0; 1.0].
Similarity CheckWaypoints(RouteParams && params, ReferenceRoutes && candidates);

/// \returns true if route from |start| to |finish| fully conforms one of |candidates|
/// and false otherwise.
bool CheckRoute(routing::VehicleType type, ms::LatLon const & start, ms::LatLon const & finish,
                std::vector<Coordinates> && referenceTracks);
bool CheckCarRoute(ms::LatLon const & start, ms::LatLon const & finish,
                   std::vector<Coordinates> && referenceTracks);
}  // namespace routing_quality
