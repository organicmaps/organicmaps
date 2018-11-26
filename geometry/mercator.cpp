#include "geometry/mercator.hpp"

#include "geometry/distance_on_sphere.hpp"

#include <algorithm>
#include <cmath>

using namespace std;

m2::RectD MercatorBounds::MetersToXY(double lon, double lat, double lonMetersR, double latMetersR)
{
  double const latDegreeOffset = latMetersR * kDegreesInMeter;
  double const minLat = max(-90.0, lat - latDegreeOffset);
  double const maxLat = min(90.0, lat + latDegreeOffset);

  double const cosL = max(cos(base::DegToRad(max(fabs(minLat), fabs(maxLat)))), 0.00001);
  ASSERT_GREATER(cosL, 0.0, ());

  double const lonDegreeOffset = lonMetersR * kDegreesInMeter / cosL;
  double const minLon = max(-180.0, lon - lonDegreeOffset);
  double const maxLon = min(180.0, lon + lonDegreeOffset);

  return m2::RectD(FromLatLon(minLat, minLon), FromLatLon(maxLat, maxLon));
}

m2::PointD MercatorBounds::GetSmPoint(m2::PointD const & pt, double lonMetersR, double latMetersR)
{
  double const lat = YToLat(pt.y);
  double const lon = XToLon(pt.x);

  double const latDegreeOffset = latMetersR * kDegreesInMeter;
  double const newLat = min(90.0, max(-90.0, lat + latDegreeOffset));

  double const cosL = max(cos(base::DegToRad(newLat)), 0.00001);
  ASSERT_GREATER(cosL, 0.0, ());

  double const lonDegreeOffset = lonMetersR * kDegreesInMeter / cosL;
  double const newLon = min(180.0, max(-180.0, lon + lonDegreeOffset));

  return FromLatLon(newLat, newLon);
}

double MercatorBounds::DistanceOnEarth(m2::PointD const & p1, m2::PointD const & p2)
{
  return ms::DistanceOnEarth(ToLatLon(p1), ToLatLon(p2));
}

double MercatorBounds::AreaOnEarth(m2::PointD const & p1, m2::PointD const & p2,
                                   m2::PointD const & p3)
{
  return ms::AreaOnEarth(ToLatLon(p1), ToLatLon(p2), ToLatLon(p3));
}
