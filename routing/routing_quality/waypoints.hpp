#pragma once

#include "routing/routes_builder/routes_builder.hpp"

#include "routing/vehicle_mask.hpp"

#include "routing/base/followed_polyline.hpp"

#include "geometry/latlon.hpp"

#include <vector>

namespace routing_quality
{
using Params = routing::routes_builder::RoutesBuilder::Params;

using Waypoints = std::vector<ms::LatLon>;

/// \brief There can be more than one reference route.
using ReferenceRoutes = std::vector<Waypoints>;

// Value in range: [0, 1]
using Similarity = double;

namespace metrics
{
Similarity CompareByNumberOfMatchedWaypoints(routing::FollowedPolyline polyline, Waypoints const & waypoints);
Similarity CompareByNumberOfMatchedWaypoints(routing::FollowedPolyline const & polyline, ReferenceRoutes && candidates);
}  // namespace metrics

/// \brief Checks how many reference waypoints the route contains.
/// \returns normalized value in range [0.0; 1.0].
Similarity CheckWaypoints(Params const & params, ReferenceRoutes && referenceRoutes);

/// \returns true if route from |start| to |finish| fully conforms one of |candidates|
/// and false otherwise.
bool CheckRoute(Params const & params, ReferenceRoutes && referenceRoutes);
bool CheckCarRoute(ms::LatLon const & start, ms::LatLon const & finish, ReferenceRoutes && referenceRoutes);
}  // namespace routing_quality
