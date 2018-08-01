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
  /// Waypoints which the route passes through.
  Coordinates m_waypoints;
  /// Value in range (0.0; 1.0] which indicates how desirable the route is.
  double m_factor = 1.0;
};

/// There can be more than one reference route.
using ReferenceRoutes = std::vector<ReferenceRoute>;

using Similarity = double;

/// Checks how many reference waypoints the route contains.
/// Returns normalized value in range [0.0; 1.0].
Similarity CheckWaypoints(RouteParams && params, ReferenceRoutes && candidates);
}  // namespace routing_quality
