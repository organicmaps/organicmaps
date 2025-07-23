#include "testing/testing.hpp"

#include "geometry/area_on_earth.hpp"
#include "geometry/distance_on_sphere.hpp"
#include "geometry/mercator.hpp"

#include "base/math.hpp"

#include <iostream>

UNIT_TEST(AreaOnEarth_Circle)
{
  double const kEarthSurfaceArea = 4.0 * math::pi * ms::kEarthRadiusMeters * ms::kEarthRadiusMeters;
  TEST_ALMOST_EQUAL_ABS(ms::CircleAreaOnEarth(math::pi * ms::kEarthRadiusMeters), kEarthSurfaceArea, 1e-1, ());

  TEST_ALMOST_EQUAL_ABS(ms::CircleAreaOnEarth(2.0 /* radiusMeters */), math::pi * 2.0 * 2.0, 1e-2, ());

  TEST_ALMOST_EQUAL_ABS(ms::CircleAreaOnEarth(2000.0 /* radiusMeters */), math::pi * 2000.0 * 2000.0, 1.0, ());
}

UNIT_TEST(AreaOnEarth_ThreePoints)
{
  double const kEarthSurfaceArea = 4.0 * math::pi * ms::kEarthRadiusMeters * ms::kEarthRadiusMeters;

  TEST_ALMOST_EQUAL_ABS(ms::AreaOnEarth({90.0, 0.0}, {0.0, 0.0}, {0.0, 90.0}), kEarthSurfaceArea / 8.0, 1e-1, ());

  TEST_ALMOST_EQUAL_ABS(ms::AreaOnEarth({90.0, 0.0}, {0.0, 90.0}, {0.0, -90.0}), kEarthSurfaceArea / 4.0, 1e-1, ());
}
