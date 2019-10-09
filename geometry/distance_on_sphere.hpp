#pragma once

#include "geometry/latlon.hpp"
#include "geometry/point3d.hpp"

#include "base/base.hpp"

// namespace ms - "math on sphere", similar to namespace m2.
namespace ms
{
double const kEarthRadiusMeters = 6378000.0;
// Distance on unit sphere between (lat1, lon1) and (lat2, lon2).
// lat1, lat2, lon1, lon2 - in degrees.
double DistanceOnSphere(double lat1Deg, double lon1Deg, double lat2Deg, double lon2Deg);

// Distance in meteres on Earth between (lat1, lon1) and (lat2, lon2).
// lat1, lat2, lon1, lon2 - in degrees.
double DistanceOnEarth(double lat1Deg, double lon1Deg, double lat2Deg, double lon2Deg);
double DistanceOnEarth(LatLon const & ll1, LatLon const & ll2);

m3::PointD GetPointOnSphere(LatLon const & ll, double sphereRadius);

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
