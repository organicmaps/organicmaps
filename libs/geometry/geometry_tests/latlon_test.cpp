#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"
#include "testing/testing.hpp"

UNIT_TEST(LatLonPointConstructorTest)
{
  m2::PointD basePoint(39.123, 42.456);
  ms::LatLon wgsPoint = mercator::ToLatLon(basePoint);
  m2::PointD resultPoint = mercator::FromLatLon(wgsPoint);
  TEST_ALMOST_EQUAL_ULPS(basePoint.x, resultPoint.x, ());
  TEST_ALMOST_EQUAL_ULPS(basePoint.y, resultPoint.y, ());
}
