#pragma once

#include "routing/base/followed_polyline.hpp"
#include "routing/route.hpp"
#include "routing/routing_callbacks.hpp"
#include "routing/vehicle_mask.hpp"

#include "geometry/latlon.hpp"
#include "geometry/point2d.hpp"

#include <vector>

namespace routing_quality
{
using Coordinates = std::vector<ms::LatLon>;

struct RouteParams
{
  /// Waypoints which the route passes through.
  Coordinates m_waypoints;
  routing::VehicleType m_type = routing::VehicleType::Car;
};

using RoutePoints = std::vector<m2::PointD>;

struct RouteResult
{
  routing::Route m_route{"" /* router */, 0 /* routeId */};
  routing::RouterResultCode m_code{routing::RouterResultCode::InternalError};
};

/// Builds the route based on |params| and returns its polyline.
routing::FollowedPolyline GetRouteFollowedPolyline(RouteParams && params);

/// Builds the route with |type| through |waypoints| and returns the route object and the router result code.
RouteResult GetRoute(RoutePoints && waypoints, routing::VehicleType type);

RoutePoints FromLatLon(Coordinates const & coords);
}  // namespace routing_quality
