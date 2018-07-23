#include "geometry/distance_on_sphere.hpp"

#include "base/math.hpp"

#include <algorithm>

using namespace std;

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
}  // namespace ms
