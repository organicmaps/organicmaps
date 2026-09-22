#include "testing/testing.hpp"

#include "base/math.hpp"
#include "geometry/distance_on_sphere.hpp"
#include "map/extrapolation/vehicle_motion.hpp"

#include <limits>

namespace vehicle_motion_tests
{
using extrapolation::VehicleMotion;

location::GpsInfo Fix(double longitude = 0.0, double bearing = 90.0)
{
  location::GpsInfo fix;
  fix.m_source = location::EAndroidNative;
  fix.m_timestamp = 1000.0;
  fix.m_latitude = 0.0;
  fix.m_longitude = longitude;
  fix.m_bearing = bearing;
  fix.m_speed = 10.0;
  return fix;
}

double EastMeters(location::GpsInfo const & fix, double origin = 0.0)
{
  return math::DegToRad(fix.m_longitude - origin) * ms::kEarthRadiusMeters;
}

UNIT_TEST(VehicleMotion_IntegratesSpeedBetweenOneHzFixes)
{
  VehicleMotion motion;
  TEST(motion.SetSpeed(10.0, 9.9, 10.0), ());
  motion.SetFix(Fix(), 10.0, 10.0);
  auto pose = motion.Predict(10.2);
  TEST(pose, ());
  TEST_ALMOST_EQUAL_ABS(EastMeters(*pose), 2.0, 0.001, ());
  motion.SetSpeed(20.0, 10.2, 10.2);
  pose = motion.Predict(10.4);
  TEST(pose, ());
  TEST_ALMOST_EQUAL_ABS(EastMeters(*pose), 6.0, 0.001, ());
  TEST_ALMOST_EQUAL_ABS(pose->m_timestamp, 1000.4, 0.001, ());
}

UNIT_TEST(VehicleMotion_TrustedZeroStopsAdvancing)
{
  VehicleMotion motion;
  motion.SetSpeed(10.0, 9.9, 10.0);
  motion.SetFix(Fix(), 10.0, 10.0);
  motion.SetSpeed(0.0, 10.3, 10.3);
  auto pose = motion.Predict(10.9);
  TEST(pose, ());
  TEST_ALMOST_EQUAL_ABS(EastMeters(*pose), 3.0, 0.001, ());
  TEST_EQUAL(pose->m_speed, 0.0, ());
}

UNIT_TEST(VehicleMotion_InvalidSpeedHoldsUntilNewFix)
{
  for (double speed : {76.0, -76.0, std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity()})
  {
    VehicleMotion motion;
    motion.SetSpeed(10.0, 9.9, 10.0);
    motion.SetFix(Fix(), 10.0, 10.0);
    TEST(motion.Predict(10.2), ());
    TEST(!motion.SetSpeed(speed, 10.3, 10.3), ());
    TEST(!motion.Predict(10.4), ());
    TEST(motion.ShouldHoldPosition(), ());
    motion.SetSpeed(10.0, 10.5, 10.5);
    TEST(!motion.Predict(10.6), ());
    motion.SetFix(Fix(), 11.0, 11.0);
    TEST(motion.Predict(11.2), ());
  }
}

UNIT_TEST(VehicleMotion_StaleSpeedAndFixesStopPrediction)
{
  VehicleMotion motion;
  motion.SetSpeed(10.0, 9.0, 9.0);
  motion.SetFix(Fix(), 10.0, 10.0);
  TEST(motion.Predict(10.5), ());
  TEST(!motion.Predict(11.1), ());
  TEST(motion.ShouldHoldPosition(), ());
  motion.SetSpeed(10.0, 11.2, 11.2);
  TEST(!motion.Predict(11.3), ("Do not bridge a gap longer than the speed validity window"));
  TEST(!motion.Predict(12.0), ());
  motion.SetFix(Fix(), 10.0, 13.0);
  TEST(!motion.Predict(13.0), ());
}

UNIT_TEST(VehicleMotion_UsesMeasurementTimeRatherThanReceiptTime)
{
  VehicleMotion motion;
  motion.SetSpeed(10.0, 9.8, 9.8);
  motion.SetSpeed(20.0, 10.2, 10.2);
  motion.SetFix(Fix(), 10.0, 10.4);
  auto pose = motion.Predict(10.4);
  TEST(pose, ());
  TEST_ALMOST_EQUAL_ABS(EastMeters(*pose), 6.0, 0.001, ());
}

UNIT_TEST(VehicleMotion_DoesNotBackfillUnknownSpeed)
{
  VehicleMotion motion;
  motion.SetFix(Fix(), 10.0, 10.0);
  motion.SetSpeed(10.0, 10.2, 10.2);
  TEST(!motion.Predict(10.5), ());
  TEST(motion.ShouldHoldPosition(), ());
}

UNIT_TEST(VehicleMotion_CourseIsMotionDirectionDuringReverse)
{
  VehicleMotion motion;
  motion.SetSpeed(-10.0, 9.9, 10.0);
  motion.SetFix(Fix(0.0, 270.0), 10.0, 10.0);
  auto pose = motion.Predict(10.5);
  TEST(pose, ());
  TEST_ALMOST_EQUAL_ABS(EastMeters(*pose), -5.0, 0.001, ());
  motion.SetSpeed(10.0, 10.6, 10.6);
  TEST(!motion.Predict(10.7), ());
  motion.SetFix(Fix(), 11.0, 11.0);
  pose = motion.Predict(11.5);
  TEST(pose, ());
  TEST_ALMOST_EQUAL_ABS(EastMeters(*pose), 5.0, 0.001, ());
}

UNIT_TEST(VehicleMotion_RequiresLocationAndCourseForMovement)
{
  VehicleMotion motion;
  motion.SetSpeed(10.0, 9.9, 10.0);
  TEST(!motion.Predict(10.1), ());
  motion.SetFix(Fix(0.0, -1.0), 10.0, 10.0);
  TEST(!motion.Predict(10.1), ());
  TEST(!motion.ShouldHoldPosition(), ());
  motion.SetSpeed(0.0, 10.2, 10.2);
  motion.SetFix(Fix(0.0, -1.0), 10.3, 10.3);
  TEST(motion.Predict(10.5), ("A valid zero does not require a course"));
}

UNIT_TEST(VehicleMotion_NextGnssFixResetsAccumulatedDistance)
{
  VehicleMotion motion;
  motion.SetSpeed(10.0, 9.9, 10.0);
  motion.SetFix(Fix(), 10.0, 10.0);
  TEST(motion.Predict(10.8), ());
  motion.SetFix(Fix(1.0), 10.9, 10.9);
  auto pose = motion.Predict(11.0);
  TEST(pose, ());
  TEST_ALMOST_EQUAL_ABS(EastMeters(*pose, 1.0), 1.0, 0.001, ());
}

UNIT_TEST(VehicleMotion_RejectsFutureAndOutOfOrderSpeed)
{
  VehicleMotion motion;
  TEST(!motion.SetSpeed(10.0, 10.1, 10.0), ());
  TEST(motion.SetSpeed(10.0, 9.9, 10.0), ());
  TEST(!motion.SetSpeed(20.0, 9.8, 10.0), ());
  motion.SetFix(Fix(), 10.0, 10.0);
  auto pose = motion.Predict(10.5);
  TEST(pose, ());
  TEST_ALMOST_EQUAL_ABS(EastMeters(*pose), 5.0, 0.001, ());
}

UNIT_TEST(VehicleMotion_DelayedBrakingDoesNotReverseThePrediction)
{
  VehicleMotion motion;
  motion.SetSpeed(10.0, 9.9, 10.0);
  motion.SetFix(Fix(), 10.0, 10.0);
  auto pose = motion.Predict(10.4);
  TEST(pose, ());
  TEST_ALMOST_EQUAL_ABS(EastMeters(*pose), 4.0, 0.001, ());
  motion.SetSpeed(0.0, 10.3, 10.5);
  pose = motion.Predict(10.6);
  TEST(pose, ());
  TEST_ALMOST_EQUAL_ABS(EastMeters(*pose), 4.0, 0.001, ());
  TEST_EQUAL(pose->m_speed, 0.0, ());
}

UNIT_TEST(VehicleMotion_NormalizesLongitudeAtTheDateLine)
{
  VehicleMotion motion;
  motion.SetSpeed(20.0, 9.9, 10.0);
  auto fix = Fix(179.9999);
  motion.SetFix(fix, 10.0, 10.0);
  auto pose = motion.Predict(10.9);
  TEST(pose, ());
  TEST_LESS(pose->m_longitude, 0.0, ());
  TEST_ALMOST_EQUAL_ABS(ms::DistanceOnEarth(fix.GetLatLon(), pose->GetLatLon()), 18.0, 0.001, ());
}
}  // namespace vehicle_motion_tests
