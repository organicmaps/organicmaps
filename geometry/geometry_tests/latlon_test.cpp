#include "testing/testing.hpp"
#include "geometry/latlon.hpp"
#include "geometry/point2d.hpp"
#include "indexer/mercator.hpp"

UNIT_TEST(LatLonPointConstructorTest)
{
  m2::PointD basePoint(39.123, 42.456);
  ms::LatLon wgsPoint(basePoint);
  m2::PointD resultPoint(MercatorBounds::LonToX(wgsPoint.lon),
                         MercatorBounds::LatToY(wgsPoint.lat));
  TEST_ALMOST_EQUAL_ULPS(basePoint.x, resultPoint.x, ());
  TEST_ALMOST_EQUAL_ULPS(basePoint.y, resultPoint.y, ());
}
