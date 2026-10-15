#pragma once

#include "geometry/latlon.hpp"

#include <optional>

// Navigation without GPS: dead reckoning on the roads, GPS spoofing detection and the manual position.
namespace nogps
{
// The radius all the constants of the navigation were tuned with.
double constexpr kEarthRadiusM = 6'371'000.0;

/// \returns the point |distanceM| away from |from| in the |bearingDeg| direction (clockwise from the north).
ms::LatLon Move(ms::LatLon const & from, double bearingDeg, double distanceM);

/// \returns meters between the points along the Earth surface.
double Distance(ms::LatLon const & a, ms::LatLon const & b);

/// \returns the bearing from |from| to |to|, degrees clockwise from the north in [0, 360).
double Bearing(ms::LatLon const & from, ms::LatLon const & to);

/// \returns meters from |point| to the piece between |from| and |to|, on a local flat projection.
double DistanceToPiece(ms::LatLon const & point, ms::LatLon const & from, ms::LatLon const & to);

/// \returns |deg| in [0, 360).
double Normalize(double deg);

/// \returns the shortest signed rotation from |from| to |to|, in [-180, 180).
double AngleDiff(double to, double from);

/// \returns the road bearing turned the way closest to |heading|: a road goes both ways. Without the heading the
/// bearing is kept.
double Orient(double roadBearingDeg, std::optional<double> headingDeg);
}  // namespace nogps
