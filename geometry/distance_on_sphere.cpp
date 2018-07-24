#include "geometry/distance_on_sphere.hpp"

#include "base/math.hpp"

#include <algorithm>
#include <cmath>

using namespace std;

namespace
{
// Earth radius in meters.
double constexpr kEarthRadiusMeters = 6378000;

// Side of the one-degree square at the equator in meters.
double constexpr kOneDegreeEquatorLengthMeters = 111319.49079;
}  // namespace

namespace ms
{
double DistanceOnSphere(double lat1Deg, double lon1Deg, double lat2Deg, double lon2Deg)
{
  double const lat1 = my::DegToRad(lat1Deg);
  double const lat2 = my::DegToRad(lat2Deg);
  double const dlat = sin((lat2 - lat1) * 0.5);
  double const dlon = sin((my::DegToRad(lon2Deg) - my::DegToRad(lon1Deg)) * 0.5);
  double const y = dlat * dlat + dlon * dlon * cos(lat1) * cos(lat2);
  return 2.0 * atan2(sqrt(y), sqrt(max(0.0, 1.0 - y)));
}

double AreaOnSphere(ms::LatLon const & ll1, ms::LatLon const & ll2, ms::LatLon const & ll3)
{
  // Todo: proper area on sphere (not needed for now)
  double const avgLat = my::DegToRad((ll1.lat + ll2.lat + ll3.lat) / 3);
  return cos(avgLat) * 0.5 *
         fabs((ll2.lon - ll1.lon) * (ll3.lat - ll1.lat) -
              (ll3.lon - ll1.lon) * (ll2.lat - ll1.lat));
}

double DistanceOnEarth(double lat1Deg, double lon1Deg, double lat2Deg, double lon2Deg)
{
  return kEarthRadiusMeters * DistanceOnSphere(lat1Deg, lon1Deg, lat2Deg, lon2Deg);
}

double DistanceOnEarth(LatLon const & ll1, LatLon const & ll2)
{
  return DistanceOnEarth(ll1.lat, ll1.lon, ll2.lat, ll2.lon);
}

double AreaOnEarth(LatLon const & ll1, LatLon const & ll2, LatLon const & ll3)
{
  return kOneDegreeEquatorLengthMeters * kOneDegreeEquatorLengthMeters *
         AreaOnSphere(ll1, ll2, ll3);
}
}  // namespace ms
