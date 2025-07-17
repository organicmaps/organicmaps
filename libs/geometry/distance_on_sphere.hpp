#pragma once

#include "geometry/latlon.hpp"

// namespace ms - "math on sphere", similar to namespace m2.
namespace ms
{
double constexpr kEarthRadiusMeters = 6378000.0;
// Distance on unit sphere between (lat1, lon1) and (lat2, lon2).
// lat1, lat2, lon1, lon2 - in degrees.
double DistanceOnSphere(double lat1Deg, double lon1Deg, double lat2Deg, double lon2Deg);

// Distance in meteres on Earth between (lat1, lon1) and (lat2, lon2).
// lat1, lat2, lon1, lon2 - in degrees.
double DistanceOnEarth(double lat1Deg, double lon1Deg, double lat2Deg, double lon2Deg);
double DistanceOnEarth(LatLon const & ll1, LatLon const & ll2);
}  // namespace ms
