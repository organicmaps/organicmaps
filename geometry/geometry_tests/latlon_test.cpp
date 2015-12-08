#include "testing/testing.hpp"
#include "geometry/latlon.hpp"
#include "geometry/point2d.hpp"
#include "geometry/mercator.hpp"

UNIT_TEST(LatLonPointConstructorTest)
{
  m2::PointD basePoint(39.123, 42.456);
  ms::LatLon wgsPoint = MercatorBounds::ToLatLon(basePoint);
  m2::PointD resultPoint = MercatorBounds::FromLatLon(wgsPoint);
  TEST_ALMOST_EQUAL_ULPS(basePoint.x, resultPoint.x, ());
  TEST_ALMOST_EQUAL_ULPS(basePoint.y, resultPoint.y, ());
}
