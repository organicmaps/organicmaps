#include "testing/testing.hpp"

#include "map/nogps/accel_log.hpp"
#include "map/nogps/dead_reckoning.hpp"
#include "map/nogps/elm327_parser.hpp"
#include "map/nogps/geo.hpp"
#include "map/nogps/gyro_calibrator.hpp"
#include "map/nogps/speed_scale.hpp"

#include "base/math.hpp"
#include "base/string_utils.hpp"

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace nogps_inertial_tests
{
using namespace nogps;

double constexpr kLat = 50.4501;
double constexpr kLon = 30.5234;
// Meters per degree of latitude.
double constexpr kMPerDeg = 111'195;

UNIT_TEST(NoGps_Elm327_ParsesSpeed)
{
  TEST_EQUAL(ParseElm327Speed("41 0D 3C\r"), 60, ());
  TEST_EQUAL(ParseElm327Speed("410D3C"), 60, ());
  TEST_EQUAL(ParseElm327Speed("SEARCHING...\r41 0D 00\r\r"), 0, ());
  TEST_EQUAL(ParseElm327Speed("41 0d ff"), 255, ());
  TEST_EQUAL(ParseElm327Speed("41 0D 3C\r41 0D 3C\r"), 60, ());
  TEST(!ParseElm327Speed("NO DATA\r"), ());
  TEST(!ParseElm327Speed("UNABLE TO CONNECT"), ());
  TEST(!ParseElm327Speed("41 0D"), ());
  TEST(!ParseElm327Speed("?"), ());
}

UNIT_TEST(NoGps_DeadReckoning_NotReadyWithoutHeading)
{
  DeadReckoning dr;
  dr.SetPosition({kLat, kLon});
  dr.SetSpeed(10);
  dr.Advance(10);
  TEST(!dr.IsReady(), ());
  TEST_EQUAL(dr.GetPosition().m_lat, kLat, ());
}

UNIT_TEST(NoGps_DeadReckoning_DrivesNorthAndEast)
{
  DeadReckoning dr;
  dr.SetPosition({kLat, kLon});
  dr.SetHeading(0);
  dr.SetSpeed(10);
  dr.Advance(100);
  TEST_ALMOST_EQUAL_ABS((dr.GetPosition().m_lat - kLat) * kMPerDeg, 1000.0, 1.0, ());
  TEST_ALMOST_EQUAL_ABS(dr.GetPosition().m_lon, kLon, 1e-9, ());

  dr.SetPosition({kLat, kLon});
  dr.SetHeading(90);
  dr.Advance(100);
  TEST_ALMOST_EQUAL_ABS(dr.GetPosition().m_lat, kLat, 1e-6, ());
  TEST_ALMOST_EQUAL_ABS((dr.GetPosition().m_lon - kLon) * kMPerDeg * std::cos(math::DegToRad(kLat)), 1000.0, 1.0, ());
}

UNIT_TEST(NoGps_DeadReckoning_DrivesBackLookingWhereItDid)
{
  DeadReckoning dr;
  dr.SetPosition({kLat, kLon});
  dr.SetHeading(0);
  dr.AdvanceBy(100);
  dr.ReverseBy(30);
  TEST_ALMOST_EQUAL_ABS((dr.GetPosition().m_lat - kLat) * kMPerDeg, 70.0, 0.1, ());
  TEST_ALMOST_EQUAL_ABS(dr.GetHeading(), 0.0, 1e-9, ());
  // The accuracy degrades with the way driven, whichever way.
  TEST_ALMOST_EQUAL_ABS(dr.GetDistanceSinceFix(), 130.0, 1e-6, ());
}

UNIT_TEST(NoGps_DeadReckoning_AccuracyGrowsWithDistance)
{
  DeadReckoning dr;
  dr.SetPosition({kLat, kLon});
  dr.SetHeading(45);
  dr.SetSpeed(20);
  dr.Advance(50);
  TEST_ALMOST_EQUAL_ABS(dr.GetDistanceSinceFix(), 1000.0, 1e-6, ());
  TEST_ALMOST_EQUAL_ABS(dr.GetAccuracy(), DeadReckoning::kBaseAccuracyM + 1000 * DeadReckoning::kAccuracyPerMeter, 1e-6,
                        ());
  dr.SetPosition({kLat, kLon});
  TEST_ALMOST_EQUAL_ABS(dr.GetAccuracy(), DeadReckoning::kBaseAccuracyM, 1e-6, ());
}

UNIT_TEST(NoGps_DeadReckoning_RotatesAndNormalizesHeading)
{
  DeadReckoning dr;
  dr.SetHeading(350);
  dr.Rotate(20);
  TEST_ALMOST_EQUAL_ABS(dr.GetHeading(), 10.0, 1e-9, ());
  dr.Rotate(-30);
  TEST_ALMOST_EQUAL_ABS(dr.GetHeading(), 340.0, 1e-9, ());
}

UNIT_TEST(NoGps_Geo_AngleDiff)
{
  TEST_ALMOST_EQUAL_ABS(AngleDiff(10, 350), 20.0, 1e-9, ());
  TEST_ALMOST_EQUAL_ABS(AngleDiff(350, 10), -20.0, 1e-9, ());
  TEST_ALMOST_EQUAL_ABS(AngleDiff(90, 90), 0.0, 1e-9, ());
}

UNIT_TEST(NoGps_DeadReckoning_SnapsToRoadAndCorrectsHeading)
{
  DeadReckoning dr;
  dr.SetPosition({kLat, kLon});
  dr.SetHeading(10);
  // The road goes to the north: the position moves to it, the heading turns a bit towards it.
  TEST(dr.SnapToRoad({kLat + 0.0001, kLon + 0.0002}, 0), ());
  TEST_ALMOST_EQUAL_ABS(dr.GetPosition().m_lat, kLat + 0.0001, 1e-12, ());
  TEST_ALMOST_EQUAL_ABS(dr.GetPosition().m_lon, kLon + 0.0002, 1e-12, ());
  double const heading = 10 - 10 * DeadReckoning::kSnapHeadingWeight;
  TEST_ALMOST_EQUAL_ABS(dr.GetHeading(), heading, 1e-9, ());
  // The same road in the opposite direction.
  TEST(dr.SnapToRoad({kLat, kLon}, 180), ());
  TEST_ALMOST_EQUAL_ABS(dr.GetHeading(), heading - heading * DeadReckoning::kSnapHeadingWeight, 1e-9, ());
}

UNIT_TEST(NoGps_DeadReckoning_KeepsHeadingOnRoadGoingAway)
{
  // A fork or a slow turn: the car is still on the road, but the gyroscope knows better where it goes.
  DeadReckoning dr;
  dr.SetPosition({kLat, kLon});
  dr.SetHeading(25);
  TEST(dr.SnapToRoad({kLat + 0.0001, kLon}, 0), ());
  TEST_ALMOST_EQUAL_ABS(dr.GetPosition().m_lat, kLat + 0.0001, 1e-12, ());
  TEST_EQUAL(dr.GetHeading(), 25, ());
}

UNIT_TEST(NoGps_DeadReckoning_TurnsHeadingAsideAlongRoadDrivenLong)
{
  // The heading was taken from a short piece of a road at a crossing, the car keeps going along the road.
  DeadReckoning dr;
  dr.SetPosition({kLat, kLon});
  dr.SetHeading(18);
  dr.SetSpeed(10);
  for (int i = 0; i < 4; ++i)
  {
    dr.Advance(1);
    TEST(dr.SnapToRoad({dr.GetPosition().m_lat, kLon}, 0), ());
  }
  TEST_EQUAL(dr.GetHeading(), 18, ());
  for (int i = 0; i < 10; ++i)
  {
    dr.Advance(1);
    TEST(dr.SnapToRoad({dr.GetPosition().m_lat, kLon}, 0), ());
  }
  TEST_LESS(dr.GetHeading(), 10, ());
}

UNIT_TEST(NoGps_DeadReckoning_IgnoresCrossingRoad)
{
  DeadReckoning dr;
  dr.SetPosition({kLat, kLon});
  dr.SetHeading(0);
  TEST(!dr.SnapToRoad({kLat + 0.001, kLon}, 90), ());
  TEST_EQUAL(dr.GetPosition().m_lat, kLat, ());
  TEST_EQUAL(dr.GetHeading(), 0, ());
}

UNIT_TEST(NoGps_Geo_MovesByBearing)
{
  auto const north = Move({kLat, kLon}, 0, 500);
  TEST_ALMOST_EQUAL_ABS((north.m_lat - kLat) * kMPerDeg, 500.0, 1.0, ());
  TEST_ALMOST_EQUAL_ABS(north.m_lon, kLon, 1e-9, ());

  auto const back = Move(north, 180, 500);
  TEST_ALMOST_EQUAL_ABS(back.m_lat, kLat, 1e-6, ());
  TEST_ALMOST_EQUAL_ABS(back.m_lon, kLon, 1e-9, ());
}

UNIT_TEST(NoGps_SpeedScale_EstimatesFromCorrections)
{
  SpeedScale scale;
  TEST_EQUAL(scale.Get(), 1, ());

  // A correction on a short distance says nothing about the speed.
  scale.OnDistance(100);
  scale.OnCorrection(50);
  TEST_EQUAL(scale.Get(), 1, ());

  // The position lagged by 150 m on 1000 m, a half of the error is compensated.
  scale.OnDistance(900);
  scale.OnCorrection(100);
  TEST_ALMOST_EQUAL_ABS(scale.Get(), 1 + 150.0 / 1000 * SpeedScale::kCorrectionWeight, 1e-9, ());

  // The position ran ahead, the speed is lowered.
  double const overestimated = scale.Get();
  scale.OnDistance(1000);
  scale.OnCorrection(-100);
  TEST_LESS(scale.Get(), overestimated, ());

  scale.Reset();
  TEST_EQUAL(scale.Get(), 1, ());
}

UNIT_TEST(NoGps_SpeedScale_Limits)
{
  SpeedScale scale;
  for (int i = 0; i < 100; ++i)
  {
    scale.OnDistance(1000);
    scale.OnCorrection(1000);
  }
  TEST_EQUAL(scale.Get(), SpeedScale::kMaxScale, ());

  for (int i = 0; i < 100; ++i)
  {
    scale.OnDistance(1000);
    scale.OnCorrection(-1000);
  }
  TEST_EQUAL(scale.Get(), SpeedScale::kMinScale, ());
}

UNIT_TEST(NoGps_SpeedScale_Restores)
{
  SpeedScale scale;
  scale.Set(1.07);
  TEST_EQUAL(scale.Get(), 1.07, ());

  // A distance driven before the restore is not mixed with the new trip.
  scale.OnDistance(200);
  scale.Set(1.07);
  scale.OnDistance(200);
  scale.OnCorrection(100);
  TEST_EQUAL(scale.Get(), 1.07, ());

  scale.Set(5);
  TEST_EQUAL(scale.Get(), SpeedScale::kMaxScale, ());
}

UNIT_TEST(NoGps_GyroCalibrator_YawRateForAnyPhoneOrientation)
{
  Vec3 const noBias = {0, 0, 0};
  // Phone lies flat screen up: turning right (clockwise from above) is a negative rotation around z.
  TEST_ALMOST_EQUAL_ABS(GyroCalibrator::YawRateDeg({0, 0, math::DegToRad(-10.0)}, noBias, {0, 0, 1}), 10.0, 1e-4, ());
  // Phone stands in portrait on the dashboard: up is the phone y axis.
  TEST_ALMOST_EQUAL_ABS(GyroCalibrator::YawRateDeg({0, math::DegToRad(-10.0), 0}, noBias, {0, 1, 0}), 10.0, 1e-4, ());
  // Rotation around a horizontal axis (bumps) is not a turn.
  TEST_ALMOST_EQUAL_ABS(GyroCalibrator::YawRateDeg({1, 0, 0}, noBias, {0, 1, 0}), 0.0, 1e-9, ());
  // The bias is removed.
  TEST_ALMOST_EQUAL_ABS(GyroCalibrator::YawRateDeg({0, 0, 0.01}, {0, 0, 0.01}, {0, 0, 1}), 0.0, 1e-9, ());
}

UNIT_TEST(NoGps_GyroCalibrator_CalibratesStillPhone)
{
  GyroCalibrator calibrator;
  for (int i = 0; i < GyroCalibrator::kRequiredSamples; ++i)
  {
    double const noise = (i % 2 == 0) ? 0.005 : -0.005;
    calibrator.Add({0.02 + noise, -0.01, 0.003}, {0, 9.81 + noise, 0.3});
  }
  TEST(calibrator.IsComplete(), ());
  TEST(calibrator.IsStill(), ());
  TEST_ALMOST_EQUAL_ABS(calibrator.GetBias()[0], 0.02, 1e-6, ());
  TEST_ALMOST_EQUAL_ABS(calibrator.GetBias()[1], -0.01, 1e-6, ());
  TEST_ALMOST_EQUAL_ABS(calibrator.GetUp()[1], 1.0, 1e-3, ());
}

UNIT_TEST(NoGps_GyroCalibrator_RejectsMovingPhone)
{
  GyroCalibrator calibrator;
  for (int i = 0; i < GyroCalibrator::kRequiredSamples; ++i)
    calibrator.Add({(i % 2 == 0) ? 0.5 : -0.5, 0, 0}, {0, 9.81, 0});
  TEST(!calibrator.IsStill(), ());
}

UNIT_TEST(NoGps_GyroCalibrator_IgnoresSlowTurnAtShortStop)
{
  Vec3 const up = {0, 0, 1};
  Vec3 const bias = {0.01, 0.02, 0.003};
  // The temperature drift of the bias.
  Vec3 const drifted = {0.01, 0.02, 0.003 + math::DegToRad(0.2)};
  // The car creeping and turning at 2.7 deg/s while the car tells it stands.
  Vec3 const turning = {0.01, 0.02, 0.003 + math::DegToRad(2.7)};
  TEST(GyroCalibrator::IsBiasChangeAllowed({}, turning, up, 3000), ());
  TEST(GyroCalibrator::IsBiasChangeAllowed(bias, drifted, up, 3000), ());
  TEST(!GyroCalibrator::IsBiasChangeAllowed(bias, turning, up, 3000), ());
  // Nobody turns for so long at a stop, the bias was wrong.
  TEST(GyroCalibrator::IsBiasChangeAllowed(bias, turning, up, GyroCalibrator::kLongStopMs), ());
}

UNIT_TEST(NoGps_GyroCalibrator_ConfirmsCalibrationByNextOne)
{
  Vec3 const up = {0, 0, 1};
  Vec3 const bias = {0.01, 0.02, 0.003};
  Vec3 const noisy = {0.01, 0.02, 0.003 + math::DegToRad(0.2)};
  // The car starts off while it is told to stand.
  Vec3 const starting = {0.01, 0.02, 0.003 + math::DegToRad(1.5)};
  auto const confirmed = GyroCalibrator::Confirm(bias, noisy, up);
  TEST(confirmed, ());
  TEST_ALMOST_EQUAL_ABS(
      GyroCalibrator::YawRateDeg({0, 0, 0}, *confirmed, up) - GyroCalibrator::YawRateDeg({0, 0, 0}, bias, up), 0.1,
      1e-3, ());
  TEST(!GyroCalibrator::Confirm(bias, starting, up), ());
}

int64_t constexpr kStepNs = 20'000'000;

UNIT_TEST(NoGps_AccelLog_LineEverySecond)
{
  AccelLog log;
  std::string line;
  int lines = 0;
  // 50 samples a second for 3 s and one more to end the last second.
  for (int i = 0; i <= 150; ++i)
  {
    // A jolt of 3 m/s² up in the third part of the second second.
    double const z = i == 75 ? 12.81 : 9.81;
    auto const result = log.OnSample(i * kStepNs, 0.5, -1, z);
    if (!result)
      continue;
    ++lines;
    if (lines == 2)
      line = *result;
  }
  TEST_EQUAL(lines, 3, ());
  auto const slices = strings::Tokenize<std::string>(line, " ");
  TEST_EQUAL(slices.size(), AccelLog::kSlices, (line));
  TEST_EQUAL(slices[0], "0.50,-1.00,9.81,0.0", ());
  // The mean of the part with the jolt is 0.3 higher, the jolt is 2.7 above it.
  TEST_EQUAL(slices[2], "0.50,-1.00,10.11,2.7", ());
}

UNIT_TEST(NoGps_AccelLog_JoltOfSensor)
{
  AccelLog log;
  std::string line;
  for (int i = 0; i <= 50 && line.empty(); ++i)
  {
    // The box has measured a jolt of 6.8 m/s² inside one of its samples of the second part.
    if (auto const result = log.OnSample(i * kStepNs, 0, 0, 0.1, i == 15 ? 6.8 : 0.2))
      line = *result;
  }
  auto const slices = strings::Tokenize<std::string>(line, " ");
  TEST_EQUAL(slices.size(), AccelLog::kSlices, (line));
  TEST_EQUAL(slices[0], "0.00,0.00,0.10,0.2", ());
  TEST_EQUAL(slices[1], "0.00,0.00,0.10,6.8", ());
}

UNIT_TEST(NoGps_GyroLog_LineEverySecond)
{
  GyroLog log;
  std::vector<std::string> lines;
  // 50 samples a second for 3 s and one more to end the last second. The car turns right by 20 degrees a second
  // in the third part of the second second, and shakes by 0.5 degrees to both sides in the third one.
  for (int i = 0; i <= 150; ++i)
  {
    double delta = 0;
    if (i >= 70 && i < 80)
      delta = 0.4;
    else if (i >= 100 && i < 150)
      delta = i % 2 == 0 ? 0.5 : -0.5;
    // The car reports 7 km/h from the middle of the second second on.
    if (auto const line = log.OnSample(i * kStepNs, delta, i < 75 ? 0 : 7))
    {
      lines.push_back(*line);
      if (lines.size() == 2)
        TEST_ALMOST_EQUAL_ABS(log.GetLineRotationDeg(), 4.0, 1e-9, ());
      else if (lines.size() == 3)
        TEST_ALMOST_EQUAL_ABS(log.GetLineRotationDeg(), 25.0, 1e-9, ());
    }
  }
  TEST_EQUAL(lines.size(), 3, ());
  TEST_EQUAL(lines[0], "0.0 0.00 0.00 0.00 0.00 0.00 0 0 0 0 0", ());
  // The total rotation since the start, the parts of the second, and the speed of the car by the end of each.
  TEST_EQUAL(lines[1], "4.0 0.00 0.00 4.00 0.00 0.00 0 0 7 7 7", ());
  TEST_EQUAL(lines[2], "4.0 0.00 0.00 0.00 0.00 0.00 7 7 7 7 7", ());

  // The sensor was silent for 5 s: the unfinished second is dropped, the total goes on.
  int64_t constexpr kLater = 10'000'000'000LL;
  for (int i = 0; i < 50; ++i)
    TEST(!log.OnSample(kLater + i * kStepNs, 0.1, -1), ());
  auto const line = log.OnSample(kLater + 50 * kStepNs, 0.1, -1);
  TEST(line, ());
  // The speed of the car is not known.
  TEST_EQUAL(*line, "9.0 1.00 1.00 1.00 1.00 1.00 -1 -1 -1 -1 -1", ());
}

UNIT_TEST(NoGps_AccelLog_BreakStartsAnew)
{
  AccelLog log;
  for (int i = 0; i < 40; ++i)
    TEST(!log.OnSample(i * kStepNs, 0, 0, 9.81), ());
  // The sensor was silent for 5 s: the unfinished second is dropped.
  int64_t constexpr kLater = 5'000'000'000LL;
  for (int i = 0; i < 50; ++i)
    TEST(!log.OnSample(kLater + i * kStepNs, 0, 0, 9.81), ());
  TEST(log.OnSample(kLater + 50 * kStepNs, 0, 0, 9.81), ());
}
}  // namespace nogps_inertial_tests
