#include "indexer/mercator.hpp"

#include "geometry/distance_on_sphere.hpp"

double MercatorBounds::minX = -180;
double MercatorBounds::maxX = 180;
double MercatorBounds::minY = -180;
double MercatorBounds::maxY = 180;

m2::RectD MercatorBounds::MetresToXY(double lon, double lat,
                                     double lonMetresR, double latMetresR)
{
  double const latDegreeOffset = latMetresR * degreeInMetres;
  double const minLat = max(-90.0, lat - latDegreeOffset);
  double const maxLat = min( 90.0, lat + latDegreeOffset);

  double const cosL = max(cos(my::DegToRad(max(fabs(minLat), fabs(maxLat)))), 0.00001);
  ASSERT_GREATER ( cosL, 0.0, () );

  double const lonDegreeOffset = lonMetresR * degreeInMetres / cosL;
  double const minLon = max(-180.0, lon - lonDegreeOffset);
  double const maxLon = min( 180.0, lon + lonDegreeOffset);

  return m2::RectD(FromLatLon(minLat, minLon), FromLatLon(maxLat, maxLon));
}

m2::PointD MercatorBounds::GetSmPoint(m2::PointD const & pt, double lonMetresR, double latMetresR)
{
  double const lat = YToLat(pt.y);
  double const lon = XToLon(pt.x);

  double const latDegreeOffset = latMetresR * degreeInMetres;
  double const newLat = min(90.0, max(-90.0, lat + latDegreeOffset));

  double const cosL = max(cos(my::DegToRad(newLat)), 0.00001);
  ASSERT_GREATER ( cosL, 0.0, () );

  double const lonDegreeOffset = lonMetresR * degreeInMetres / cosL;
  double const newLon = min(180.0, max(-180.0, lon + lonDegreeOffset));

  return FromLatLon(newLat, newLon);
}

double MercatorBounds::DistanceOnEarth(m2::PointD const & p1, m2::PointD const & p2)
{
  return ms::DistanceOnEarth(YToLat(p1.y), XToLon(p1.x), YToLat(p2.y), XToLon(p2.x));
}
