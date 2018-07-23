#pragma once

#include "geometry/latlon.hpp"

#include "base/base.hpp"

// namespace ms - "math on sphere", similar to the namespaces m2 and mn.
namespace ms
{
// Earth radius in meters.
inline double EarthRadiusMeters() { return 6378000; }
// Length of one degree square at the equator in meters.
inline double OneDegreeEquatorLengthMeters() { return 111319.49079; }

// Distance on unit sphere between (lat1, lon1) and (lat2, lon2).
// lat1, lat2, lon1, lon2 - in degrees.
double DistanceOnSphere(double lat1Deg, double lon1Deg, double lat2Deg, double lon2Deg);

// Area on unit sphere for a triangle (ll1, ll2, ll3).
double AreaOnSphere(LatLon const & ll1, LatLon const & ll2, LatLon const & ll3);

// Distance in meteres on Earth between (lat1, lon1) and (lat2, lon2).
// lat1, lat2, lon1, lon2 - in degrees.
inline double DistanceOnEarth(double lat1Deg, double lon1Deg, double lat2Deg, double lon2Deg)
{
  return EarthRadiusMeters() * DistanceOnSphere(lat1Deg, lon1Deg, lat2Deg, lon2Deg);
}

inline double DistanceOnEarth(LatLon const & ll1, LatLon const & ll2)
{
  return DistanceOnEarth(ll1.lat, ll1.lon, ll2.lat, ll2.lon);
}

inline double AreaOnEarth(LatLon const & ll1, LatLon const & ll2, LatLon const & ll3)
{
  return OneDegreeEquatorLengthMeters() * OneDegreeEquatorLengthMeters() *
         AreaOnSphere(ll1, ll2, ll3);
}
}  // namespace ms
