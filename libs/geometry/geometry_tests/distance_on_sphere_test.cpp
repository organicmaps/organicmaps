#include "testing/testing.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "base/math.hpp"

UNIT_TEST(DistanceOnSphere)
{
  TEST_LESS(fabs(ms::DistanceOnSphere(0, -180, 0, 180)), 1.0e-6, ());
  TEST_LESS(fabs(ms::DistanceOnSphere(30, 0, 30, 360)), 1.0e-6, ());
  TEST_LESS(fabs(ms::DistanceOnSphere(-30, 23, -30, 23)), 1.0e-6, ());
  TEST_LESS(fabs(ms::DistanceOnSphere(90, 0, 90, 120)), 1.0e-6, ());
  TEST_LESS(fabs(ms::DistanceOnSphere(0, 0, 0, 180) - math::pi), 1.0e-6, ());
  TEST_LESS(fabs(ms::DistanceOnSphere(90, 0, -90, 120) - math::pi), 1.0e-6, ());
}

UNIT_TEST(DistanceOnEarth)
{
  TEST_LESS(fabs(ms::DistanceOnEarth(30, 0, 30, 180) * 0.001 - 13358), 1, ());
  TEST_LESS(fabs(ms::DistanceOnEarth(30, 0, 30, 45) * 0.001 - 4309), 1, ());
  TEST_LESS(fabs(ms::DistanceOnEarth(-30, 0, -30, 45) * 0.001 - 4309), 1, ());
  TEST_LESS(fabs(ms::DistanceOnEarth(47.37, 8.56, 53.91, 27.56) * 0.001 - 1519), 1, ());
  TEST_LESS(fabs(ms::DistanceOnEarth(43, 132, 38, -122.5) * 0.001 - 8302), 1, ());
}
