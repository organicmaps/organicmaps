#include "testing/testing.hpp"

#include "geometry/latlon.hpp"
#include "geometry/oblate_spheroid.hpp"

namespace
{
double constexpr kAccuracyEps = 1e-3;

void testDistance(ms::LatLon const & a, ms::LatLon const & b, double planDistance)
{
  double const factDistance = oblate_spheroid::GetDistance(a, b);
  TEST_ALMOST_EQUAL_ABS(factDistance, planDistance, kAccuracyEps, ());
}
}  // namespace

UNIT_TEST(Distance_EdgeCaseEquatorialLine)
{
  ms::LatLon const a(0.0, 0.0);
  ms::LatLon const b(0.0, 45.0);
  testDistance(a, b, 5009377.085);
}

UNIT_TEST(Distance_EdgeCaseSameLatitude)
{
  ms::LatLon const a(30.0, 30.0);
  ms::LatLon const b(30.0, 70.0);
  testDistance(a, b, 3839145.440);
}

UNIT_TEST(Distance_EdgeCaseSameLongtitude)
{
  ms::LatLon const a(10.0, 40.0);
  ms::LatLon const b(21.0, 40.0);
  testDistance(a, b, 1217222.035);
}

UNIT_TEST(Distance_Long)
{
  ms::LatLon const a(-24.02861, 123.53353);
  ms::LatLon const b(58.25020, -6.54459);
  testDistance(a, b, 14556482.656);
}

UNIT_TEST(Distance_NearlyAntipodal)
{
  ms::LatLon const a(52.02247, 168.18196);
  ms::LatLon const b(31.22321, -171.07584);
  testDistance(a, b, 2863337.631);
}

UNIT_TEST(Distance_Small)
{
  ms::LatLon const a(54.15820, 36.95131);
  ms::LatLon const b(54.15814, 36.95143);
  testDistance(a, b, 10.29832);
}

UNIT_TEST(Distance_ReallyTiny)
{
  ms::LatLon const a(-34.39292, -71.16413);
  ms::LatLon const b(-34.39294, -71.16410);
  testDistance(a, b, 3.54013);
}

UNIT_TEST(Distance_Fallback)
{
  ms::LatLon const a(0.0, 0.0);
  ms::LatLon const b(0.5, 179.5);
  testDistance(a, b, 19958365.368);
}
