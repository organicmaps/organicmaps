#include "mercator.hpp"

double MercatorBounds::minX = -180;
double MercatorBounds::maxX = 180;
double MercatorBounds::minY = -180;
double MercatorBounds::maxY = 180;
double const MercatorBounds::degreeInMetres = 360.0 / 40008245;

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
