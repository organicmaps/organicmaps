#pragma once

#include "geometry/latlon.hpp"

namespace ms
{
// Returns area of triangle on earth.
double AreaOnEarth(LatLon const & ll1, LatLon const & ll2, LatLon const & ll3);

// Area of the spherical cap that contains all points
// within the distance |radius| from an arbitrary fixed point, measured
// along the Earth surface.
// In particular, the smallest cap spanning the whole Earth results
// from radius = pi*EarthRadius.
// For small enough radiuses, returns the value close to pi*|radius|^2.
double CircleAreaOnEarth(double radius);
}  // namespace ms
