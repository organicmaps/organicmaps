#include "testing/testing.hpp"

#include "map/framework.hpp"

#include "base/math.hpp"

namespace framework_direction_tests
{
UNIT_TEST(Framework_DirectionAcrossAntimeridian)
{
  struct TestCase
  {
    double m_fromLongitude;
    m2::PointD m_target;
    double m_north;
    double m_expectedAzimuth;
  };
  TestCase const cases[] = {
      {0.0, {0.2, 0.0}, 0.0, math::pi2},      {0.2, {0.0, 0.0}, 0.0, 3 * math::pi2},
      {179.9, {-179.9, 0.0}, 0.0, math::pi2}, {-179.9, {179.9, 0.0}, 0.0, 3 * math::pi2},
      {179.9, {-179.9, 0.0}, math::pi2, 0.0}, {-179.9, {179.9, 0.0}, math::pi2, math::pi},
      {179.9, {-179.9, 0.2}, 0.0, math::pi4},
  };

  Framework framework({}, false /* loadMaps */);
  for (auto const & test : cases)
  {
    platform::Distance distance;
    double azimuth = -1.0;
    framework.GetDistanceAndAzimut(test.m_target, 0.0, test.m_fromLongitude, test.m_north, distance, azimuth);
    TEST_ALMOST_EQUAL_ABS(azimuth, test.m_expectedAzimuth, 1e-6, (test.m_fromLongitude, test.m_target, test.m_north));
  }
}
}  // namespace framework_direction_tests
