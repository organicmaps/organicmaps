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

UNIT_TEST(CircleAreaOnEarth)
{
  double const kEarthSurfaceArea = 4.0 * math::pi * ms::kEarthRadiusMeters * ms::kEarthRadiusMeters;
  TEST_ALMOST_EQUAL_ABS(ms::CircleAreaOnEarth(math::pi * ms::kEarthRadiusMeters),
                        kEarthSurfaceArea,
                        1e-1, ());

  TEST_ALMOST_EQUAL_ABS(ms::CircleAreaOnEarth(2.0 /* radiusMeters */),
                        math::pi * 2.0 * 2.0,
                        1e-2, ());

  TEST_ALMOST_EQUAL_ABS(ms::CircleAreaOnEarth(2000.0 /* radiusMeters */),
                        math::pi * 2000.0 * 2000.0,
                        1.0, ());

  double constexpr kVeryBigRadius = 1e100;
  CHECK_GREATER(kVeryBigRadius, ms::kEarthRadiusMeters, ());
  TEST_ALMOST_EQUAL_ABS(ms::CircleAreaOnEarth(kVeryBigRadius),
                        kEarthSurfaceArea,
                        1e-1, ());
}

UNIT_TEST(AreaOnEarth_ThreePoints)
{
  double const kEarthSurfaceArea = 4.0 * math::pi * ms::kEarthRadiusMeters * ms::kEarthRadiusMeters;

  TEST_ALMOST_EQUAL_ABS(ms::AreaOnEarth({90.0, 0.0}, {0.0, 0.0}, {0.0, 90.0}),
                        kEarthSurfaceArea / 8.0,
                        1e-2, ());

  TEST_ALMOST_EQUAL_ABS(ms::AreaOnEarth({90.0, 0.0}, {0.0, 90.0}, {0.0, -90.0}),
                        kEarthSurfaceArea / 4.0,
                        1e-1, ());
}
