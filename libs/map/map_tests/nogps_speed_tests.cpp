#include "testing/testing.hpp"

#include "map/nogps/accel_speed.hpp"
#include "map/nogps/speed_lag.hpp"
#include "map/nogps/speed_table.hpp"

#include <algorithm>
#include <cmath>

namespace nogps_speed_tests
{
using namespace nogps;

// The car accelerates to 15 m/s in 10 s, drives 10 s, brakes in 10 s and stands 10 s.
double TrueSpeed(double timeSec)
{
  double const t = std::fmod(std::fmod(timeSec, 40) + 40, 40);
  if (t < 10)
    return 1.5 * t;
  if (t < 20)
    return 15;
  if (t < 30)
    return 1.5 * (30 - t);
  return 0;
}

// Drives for the time: the car reports its speed 5 times a second rounded to 1 km/h and late by the lag,
// GPS measures the real speed every second.
void DriveWithLag(SpeedLag & lag, double lagSec, int seconds)
{
  for (int ms = 0; ms < seconds * 1000; ms += 200)
  {
    lag.OnCarSpeed(ms, std::floor(TrueSpeed(ms / 1000.0 - lagSec) * 3.6) / 3.6);
    if (ms % 1000 == 0)
      lag.OnGpsSpeed(ms, TrueSpeed(ms / 1000.0));
  }
}

UNIT_TEST(NoGps_SpeedLag_UsualLagUntilMeasured)
{
  SpeedLag lag;
  TEST(!lag.IsMeasured(), ());
  TEST_EQUAL(lag.Get(), SpeedLag::kDefaultLagSec, ());
  // A steady speed tells nothing.
  for (int ms = 0; ms < 600'000; ms += 200)
  {
    lag.OnCarSpeed(ms, 15);
    if (ms % 1000 == 0)
      lag.OnGpsSpeed(ms, 15);
  }
  TEST(!lag.IsMeasured(), ());
}

UNIT_TEST(NoGps_SpeedLag_MeasuresLag)
{
  for (double const lagSec : {0.4, 1.0, 1.4, 2.2})
  {
    SpeedLag lag;
    DriveWithLag(lag, lagSec, 400);
    TEST(lag.IsMeasured(), (lagSec));
    TEST_ALMOST_EQUAL_ABS(lag.Get(), lagSec, 0.2, ());
  }
}

UNIT_TEST(NoGps_SpeedLag_FollowsChangedLag)
{
  SpeedLag lag;
  DriveWithLag(lag, 0.5, 400);
  // Another adapter.
  DriveWithLag(lag, 1.5, 6000);
  TEST_ALMOST_EQUAL_ABS(lag.Get(), 1.5, 0.2, ());
}

UNIT_TEST(NoGps_SpeedLag_IsKeptBetweenTrips)
{
  SpeedLag lag;
  DriveWithLag(lag, 1.6, 400);
  SpeedLag restored;
  restored.Deserialize(lag.Serialize());
  TEST(restored.IsMeasured(), ());
  TEST_ALMOST_EQUAL_ABS(restored.Get(), lag.Get(), 0.01, ());
  restored.Deserialize("broken");
  TEST(!restored.IsMeasured(), ());
  restored.Deserialize(lag.Serialize());
  restored.Clear();
  TEST_EQUAL(restored.Get(), SpeedLag::kDefaultLagSec, ());
}

// Drives at a steady speed for |seconds|, GPS tells the real speed.
void DriveSteady(SpeedTable & table, double carKmh, double realKmh, int seconds)
{
  for (int i = 0; i < seconds; ++i)
    table.OnSample(carKmh, realKmh / 3.6, 1, 0);
}

UNIT_TEST(NoGps_SpeedTable_UnknownWithoutSamples)
{
  TEST(!SpeedTable().GetScale(50), ());
}

UNIT_TEST(NoGps_SpeedTable_MeasuresEveryRange)
{
  SpeedTable table;
  // The speedometer is 5% optimistic at 50 km/h and 2% at 90 km/h.
  DriveSteady(table, 52.5, 50, 60);
  DriveSteady(table, 91.8, 90, 60);
  TEST_ALMOST_EQUAL_ABS(*table.GetScale(55), 50 / 52.5, 1e-9, ());
  TEST_ALMOST_EQUAL_ABS(*table.GetScale(95), 90 / 91.8, 1e-9, ());
  TEST_EQUAL(table.GetKnownRanges(), 2, ());
}

UNIT_TEST(NoGps_SpeedTable_InterpolatesUnknownRanges)
{
  SpeedTable table;
  DriveSteady(table, 30, 27, 60);
  DriveSteady(table, 70, 70, 60);
  // 50 km/h is in the middle between the measured 30 and 70 km/h ranges.
  TEST_ALMOST_EQUAL_ABS(*table.GetScale(50), (0.9 + 1.0) / 2, 1e-9, ());
  // Outside the measured ranges the closest one is used.
  TEST_ALMOST_EQUAL_ABS(*table.GetScale(10), 0.9, 1e-9, ());
  TEST_ALMOST_EQUAL_ABS(*table.GetScale(120), 1.0, 1e-9, ());
}

UNIT_TEST(NoGps_SpeedTable_NeedsDistance)
{
  SpeedTable table;
  // 100 m only.
  DriveSteady(table, 36, 30, 10);
  TEST(!table.GetScale(36), ());
}

UNIT_TEST(NoGps_SpeedTable_IgnoresBadSamples)
{
  SpeedTable table;
  for (int i = 0; i < 100; ++i)
  {
    // Too slow, accelerating, after a pause.
    table.OnSample(3, 1, 1, 0);
    table.OnSample(50, 20, 1, 2);
    table.OnSample(50, 20, 5, 0);
  }
  TEST_EQUAL(table.GetKnownRanges(), 0, ());
}

UNIT_TEST(NoGps_SpeedTable_ForgetsOldSamples)
{
  SpeedTable table;
  DriveSteady(table, 60, 54, 600);
  // Much longer with the new tyres: the old ratio fades away.
  DriveSteady(table, 60, 60, 3000);
  TEST_ALMOST_EQUAL_ABS(*table.GetScale(60), 1.0, 0.02, ());
}

UNIT_TEST(NoGps_SpeedTable_KeepsBetweenTrips)
{
  SpeedTable table;
  DriveSteady(table, 52.5, 50, 60);
  SpeedTable restored;
  restored.Deserialize(table.Serialize());
  TEST_ALMOST_EQUAL_ABS(*restored.GetScale(50), *table.GetScale(50), 1e-3, ());

  restored.Deserialize("broken");
  TEST_EQUAL(restored.GetKnownRanges(), 0, ());
}
// An accelerometer fixed in a car somehow: its axes are turned against the car, the gravity is in it.
struct CarAccelerometer
{
  // The direction along the car and the vertical in the axes of the sensor.
  AccelSpeed::Vec3 m_forward = {0.8, -0.48, 0.36};
  AccelSpeed::Vec3 m_up = {0, 0.6, 0.8};
  // The part of the gravity along the car on a slope, m/s².
  double m_slope = 0;
  double m_lagSec = 1.2;
  int m_ms = 0;
  // How many times the accelerometer told the speed gain, and its largest error, m/s.
  int m_gains = 0;
  double m_maxError = 0;

  // Drives for the time: the sensor is read 50 times a second, the car reports its speed 5 times a second,
  // rounded to 1 km/h and late by the lag.
  void Drive(AccelSpeed & accel, int seconds)
  {
    m_gains = 0;
    m_maxError = 0;
    accel.SetLag(m_lagSec);
    for (int const end = m_ms + seconds * 1000; m_ms < end; m_ms += 20)
    {
      double const t = m_ms / 1000.0;
      double const along = (TrueSpeed(t + 0.01) - TrueSpeed(t - 0.01)) / 0.02 + m_slope;
      AccelSpeed::Vec3 a;
      for (size_t i = 0; i < 3; ++i)
        a[i] = m_forward[i] * along + m_up[i] * 9.81;
      accel.OnAccel(m_ms, a);
      if (m_ms % 200 == 0)
        accel.OnCarSpeed(m_ms, std::floor(TrueSpeed(t - m_lagSec) * 3.6) / 3.6);
      if (auto const gain = accel.GetGain(m_ms))
      {
        ++m_gains;
        m_maxError = std::max(m_maxError, std::fabs(*gain - (TrueSpeed(t) - TrueSpeed(t - m_lagSec))));
      }
    }
  }
};

UNIT_TEST(NoGps_AccelSpeed_NothingUntilLearned)
{
  AccelSpeed accel;
  CarAccelerometer car;
  TEST(!accel.IsUsable(), ());
  TEST(!accel.GetGain(0), ());
  // The car has sped up once: too little to know where the sensor looks.
  car.Drive(accel, 20);
  TEST(!accel.IsUsable(), ());
  TEST_EQUAL(car.m_gains, 0, ());
}

UNIT_TEST(NoGps_AccelSpeed_TellsSpeedGain)
{
  AccelSpeed accel;
  CarAccelerometer car;
  car.Drive(accel, 200);
  TEST(accel.IsUsable(), ());
  TEST_GREATER(accel.GetCorrelation(), 0.95, ());
  // Learned: the car gains 1.8 m/s during the lag while it speeds up, the accelerometer tells that.
  car.Drive(accel, 200);
  TEST_GREATER(car.m_gains, 9000, ());
  TEST_LESS(car.m_maxError, 0.25, ());
}

UNIT_TEST(NoGps_AccelSpeed_KeepsBetweenStarts)
{
  AccelSpeed accel;
  CarAccelerometer car;
  car.Drive(accel, 200);
  TEST(accel.IsUsable(), ());

  // The application is started again: the sensor box is where it was.
  AccelSpeed restored;
  TEST(restored.Deserialize(accel.Serialize()), ());
  TEST(restored.IsUsable(), ());
  TEST_ALMOST_EQUAL_ABS(restored.GetCorrelation(), accel.GetCorrelation(), 1e-6, ());
  // It tells the speed gain as soon as it has the last seconds of the sensor and of the car speed.
  car.Drive(restored, 20);
  car.Drive(restored, 60);
  TEST_GREATER(car.m_gains, 2000, ());
  TEST_LESS(car.m_maxError, 0.25, ());

  TEST(!restored.Deserialize("broken"), ());
  TEST(!restored.Deserialize(""), ());
  TEST(!restored.Deserialize("1;2;3"), ());
  TEST(restored.IsUsable(), ());
}

UNIT_TEST(NoGps_AccelSpeed_ManeuveringCarIsNotLearnedFrom)
{
  AccelSpeed accel;
  accel.SetLearning(false);
  CarAccelerometer car;
  car.Drive(accel, 200);
  TEST(!accel.IsUsable(), ());
  TEST_EQUAL(accel.GetSamples(), 0.0, ());
  accel.SetLearning(true);
  car.Drive(accel, 200);
  TEST(accel.IsUsable(), ());
}

UNIT_TEST(NoGps_AccelSpeed_SlopeDoesNotMatter)
{
  AccelSpeed accel;
  CarAccelerometer car;
  // A hill of 5 degrees all the way.
  car.m_slope = 0.85;
  car.Drive(accel, 200);
  TEST(accel.IsUsable(), ());
  car.Drive(accel, 200);
  TEST_GREATER(car.m_gains, 9000, ());
  TEST_LESS(car.m_maxError, 0.25, ());
  // The hill ends: the slope is forgotten in several seconds.
  car.m_slope = 0;
  car.Drive(accel, 20);
  car.Drive(accel, 200);
  TEST_LESS(car.m_maxError, 0.25, ());
}

UNIT_TEST(NoGps_AccelSpeed_MovedSensorIsLearnedAnew)
{
  AccelSpeed accel;
  CarAccelerometer car;
  car.Drive(accel, 200);
  TEST(accel.IsUsable(), ());
  // The phone is turned in its holder: what was forward is to the left now.
  car.m_forward = {0.6, 0.64, -0.48};
  bool lost = false;
  for (int i = 0; i < 40 && !lost; ++i)
  {
    car.Drive(accel, 1);
    lost = !accel.IsUsable();
  }
  TEST(lost, ());
  car.Drive(accel, 200);
  TEST(accel.IsUsable(), ());
  car.Drive(accel, 200);
  TEST_GREATER(car.m_gains, 9000, ());
  TEST_LESS(car.m_maxError, 0.25, ());
}

UNIT_TEST(NoGps_AccelSpeed_SilentSensorTellsNothing)
{
  AccelSpeed accel;
  CarAccelerometer car;
  car.Drive(accel, 200);
  TEST(accel.GetGain(car.m_ms - 20), ());
  // The sensor is silent for 2 s, the car reports its speed.
  for (int i = 0; i < 10; ++i)
  {
    car.m_ms += 200;
    accel.OnCarSpeed(car.m_ms, 10);
  }
  TEST(!accel.GetGain(car.m_ms), ());
}
}  // namespace nogps_speed_tests
