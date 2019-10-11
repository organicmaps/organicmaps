#include "geometry/distance_on_sphere.hpp"

#include "geometry/point3d.hpp"

#include "base/assert.hpp"
#include "base/math.hpp"

#include <algorithm>
#include <cmath>

using namespace std;

namespace ms
{
double DistanceOnSphere(double lat1Deg, double lon1Deg, double lat2Deg, double lon2Deg)
{
  double const lat1 = base::DegToRad(lat1Deg);
  double const lat2 = base::DegToRad(lat2Deg);
  double const dlat = sin((lat2 - lat1) * 0.5);
  double const dlon = sin((base::DegToRad(lon2Deg) - base::DegToRad(lon1Deg)) * 0.5);
  double const y = dlat * dlat + dlon * dlon * cos(lat1) * cos(lat2);
  return 2.0 * atan2(sqrt(y), sqrt(max(0.0, 1.0 - y)));
}

double DistanceOnEarth(double lat1Deg, double lon1Deg, double lat2Deg, double lon2Deg)
{
  return kEarthRadiusMeters * DistanceOnSphere(lat1Deg, lon1Deg, lat2Deg, lon2Deg);
}

double DistanceOnEarth(LatLon const & ll1, LatLon const & ll2)
{
  return DistanceOnEarth(ll1.m_lat, ll1.m_lon, ll2.m_lat, ll2.m_lon);
}

m3::PointD GetPointOnSphere(ms::LatLon const & ll, double sphereRadius)
{
  ASSERT(LatLon::kMinLat <= ll.m_lat && ll.m_lat <= LatLon::kMaxLat, (ll));
  ASSERT(LatLon::kMinLon <= ll.m_lon && ll.m_lon <= LatLon::kMaxLon, (ll));

  double const latRad = base::DegToRad(ll.m_lat);
  double const lonRad = base::DegToRad(ll.m_lon);

  double const x = sphereRadius * cos(latRad) * cos(lonRad);
  double const y = sphereRadius * cos(latRad) * sin(lonRad);
  double const z = sphereRadius * sin(latRad);

  return {x, y, z};
}
}  // namespace ms
