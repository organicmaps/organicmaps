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

double MercatorBounds::AreaOnEarth(m2::RectD const & rect)
{
  return MercatorBounds::AreaOnEarth(rect.LeftTop(), rect.LeftBottom(), rect.RightBottom()) +
         MercatorBounds::AreaOnEarth(rect.LeftTop(), rect.RightTop(), rect.RightBottom());
}

m2::PointD MercatorBounds::UKCoordsToXY(double eastingM, double northingM)
{
  // The map projection used on Ordnance Survey Great Britain maps is known as the National Grid.
  // It's UTM-like coordinate system.
  // The Transverse Mercator eastings and northings axes are given a ‘false origin’ just south west
  // of the Scilly Isles to ensure that all coordinates in Britain are positive. The false origin is
  // 400 km west and 100 km north of the ‘true origin’ on the central meridian at 49°N 2°W (OSGB36)
  // and approx. 49°N 2°0′5″ W (WGS 84). For further details see:
  //   https://www.ordnancesurvey.co.uk/documents/resources/guide-coordinate-systems-great-britain.pdf
  //   https://en.wikipedia.org/wiki/Ordnance_Survey_National_Grid
  auto static kNationalGridOriginX = MercatorBounds::LonToX(-7.5571597);
  auto static kNationalGridOriginY = MercatorBounds::LatToY(49.7668072);

  return GetSmPoint({kNationalGridOriginX, kNationalGridOriginY}, eastingM, northingM);
}
