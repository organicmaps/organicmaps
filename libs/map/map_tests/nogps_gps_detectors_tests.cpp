#include "testing/testing.hpp"

#include "map/nogps/geo.hpp"
#include "map/nogps/gps_return_detector.hpp"
#include "map/nogps/gps_spoofing_detector.hpp"

#include <cstdint>
#include <optional>

namespace nogps_gps_detectors_tests
{
using namespace nogps;

ms::LatLon const kKyiv(50.4501, 30.5234);
ms::LatLon const kLima(-12.0453, -77.0569);
// ~111 m per 0.001 degree of latitude.
double constexpr kStep = 0.001;

ms::LatLon North(ms::LatLon const & p, double degrees)
{
  return {p.m_lat + degrees, p.m_lon};
}

UNIT_TEST(NoGps_Geo_Distance)
{
  TEST_ALMOST_EQUAL_ABS(Distance({0, 0}, {1, 0}), 111'195.0, 10.0, ());
}

UNIT_TEST(NoGps_Spoofing_TrustsGpsNearNetworkPosition)
{
  GpsSpoofingDetector detector;
  detector.OnNetworkPosition(kKyiv, 70, 0);
  TEST(detector.CheckSatellitePosition(North(kKyiv, kStep), 1000), ());
  TEST(!detector.IsSpoofed(), ());
}

UNIT_TEST(NoGps_Spoofing_IgnoresSingleWrongNetworkPosition)
{
  GpsSpoofingDetector detector;
  TEST(detector.OnNetworkPosition(kKyiv, 70, 0), ());
  TEST(detector.CheckSatellitePosition(kKyiv, 1000), ());
  // A Wi-Fi point 8 km away, reported by two providers.
  TEST(!detector.OnNetworkPosition(North(kKyiv, 70 * kStep), 200, 1500), ());
  TEST(!detector.OnNetworkPosition(North(kKyiv, 70 * kStep), 200, 1500), ());
  TEST(detector.CheckSatellitePosition(kKyiv, 2000), ());
  // Another point near it confirms it: GPS is spoofed.
  TEST(detector.OnNetworkPosition(North(kKyiv, 71 * kStep), 200, 2500), ());
  TEST(!detector.CheckSatellitePosition(kKyiv, 3000), ());
}

UNIT_TEST(NoGps_Spoofing_IgnoresCellTowerAtWrongPoint)
{
  GpsSpoofingDetector detector;
  TEST(detector.OnNetworkPosition(kKyiv, 90, 0), ());
  // The same wrong point 8 km away again and again: the phone lies still, the marker must not fly there.
  int64_t time = 0;
  for (int i = 0; i < 5; ++i)
    TEST(!detector.OnNetworkPosition(North(kKyiv, 70 * kStep), 200, time += 20'000), ());
  // A fused position at home is not spoofed.
  TEST(detector.CheckSatellitePosition(kKyiv, time += 1000), ());
  TEST(detector.OnNetworkPosition(kKyiv, 100, time += 8000), ());
  // The point jumped away and back, it is ignored later even after a long pause.
  TEST(!detector.OnNetworkPosition(North(kKyiv, 70 * kStep), 200, time += 2 * GpsSpoofingDetector::kJumpConfirmMs), ());
  TEST(!detector.IsSpoofed(), ());
}

UNIT_TEST(NoGps_Spoofing_RemembersWrongPointsAfterRestart)
{
  GpsSpoofingDetector detector;
  TEST(detector.OnNetworkPosition(kKyiv, 90, 0), ());
  TEST(!detector.OnNetworkPosition(North(kKyiv, 70 * kStep), 200, 20'000), ());
  TEST(detector.OnNetworkPosition(kKyiv, 90, 40'000), ());

  // After a restart the wrong point is the first and the only one.
  GpsSpoofingDetector restarted;
  restarted.DeserializePhantoms(detector.SerializePhantoms());
  TEST_EQUAL(restarted.GetPhantomsCount(), 1, ());
  TEST(!restarted.OnNetworkPosition(North(kKyiv, 70 * kStep), 200, 0), ());
}

UNIT_TEST(NoGps_Spoofing_DetectsSpoofingRightAfterRestart)
{
  GpsSpoofingDetector detector;
  TEST(detector.OnNetworkPosition(kKyiv, 90, 0), ());
  auto const trusted = detector.GetLastTrusted();
  TEST(trusted, ());

  // Restarted an hour later, the network gives nothing usable and the fused position is still spoofed.
  GpsSpoofingDetector restarted;
  restarted.RestoreTrusted(trusted->m_position, trusted->m_timeMs - 3'600'000);
  TEST(!restarted.CheckSatellitePosition(kLima, 0), ());
  TEST(restarted.IsSpoofed(), ());
}

UNIT_TEST(NoGps_Spoofing_PrefersWiFiToCellTowerAtWrongPoint)
{
  GpsSpoofingDetector detector;
  // In the morning the first position is from a cell tower at a wrong point, then Wi-Fi at home comes.
  TEST(detector.OnNetworkPosition(North(kKyiv, 70 * kStep), 530, 0), ());
  TEST(detector.OnNetworkPosition(kKyiv, 50, 3000), ());
  TEST(detector.CheckSatellitePosition(kKyiv, 4000), ());
}

UNIT_TEST(NoGps_Spoofing_AcceptsJumpNothingContradicts)
{
  GpsSpoofingDetector detector;
  TEST(detector.OnNetworkPosition(kKyiv, 90, 0), ());
  // Only one cell tower far away, e.g. the phone was moved while switched off.
  int64_t time = 1000;
  TEST(!detector.OnNetworkPosition(North(kKyiv, 70 * kStep), 200, time), ());
  TEST(!detector.OnNetworkPosition(North(kKyiv, 70 * kStep), 200, time += 20'000), ());
  TEST(detector.OnNetworkPosition(North(kKyiv, 70 * kStep), 200, time += GpsSpoofingDetector::kJumpConfirmMs), ());
}

UNIT_TEST(NoGps_Spoofing_FollowsNetworkPositionsOfMovingCar)
{
  GpsSpoofingDetector detector;
  int64_t time = 0;
  // 60 km/h, a network position every 20 s.
  for (int i = 0; i < 20; ++i)
    TEST(detector.OnNetworkPosition(North(kKyiv, 3 * i * kStep), 100, time += 20'000), (i));
}

UNIT_TEST(NoGps_Spoofing_DetectsGpsOnAnotherContinent)
{
  GpsSpoofingDetector detector;
  detector.OnNetworkPosition(kKyiv, 70, 0);
  TEST(!detector.CheckSatellitePosition(kLima, 1000), ());
  TEST(detector.IsSpoofed(), ());
}

UNIT_TEST(NoGps_Spoofing_TrustsGpsWithoutAnyReference)
{
  GpsSpoofingDetector detector;
  TEST(detector.CheckSatellitePosition(kKyiv, 0), ());
}

UNIT_TEST(NoGps_Spoofing_DetectsJumpWithoutNetwork)
{
  GpsSpoofingDetector detector;
  TEST(detector.CheckSatellitePosition(kKyiv, 0), ());
  // Driving: ~111 m per 5 s is ~80 km/h.
  TEST(detector.CheckSatellitePosition(North(kKyiv, kStep), 5000), ());
  // 50 km in 10 s.
  TEST(!detector.CheckSatellitePosition(North(kKyiv, 450 * kStep), 15'000), ());
  TEST(detector.IsSpoofed(), ());
}

UNIT_TEST(NoGps_Spoofing_IgnoresStaleNetworkPosition)
{
  GpsSpoofingDetector detector;
  detector.OnNetworkPosition(kLima, 70, 0);
  // The network position is too old and a user could drive far away since then.
  TEST(detector.CheckSatellitePosition(kKyiv, GpsSpoofingDetector::kNetworkMaxAgeMs + 1), ());
}

UNIT_TEST(NoGps_Spoofing_RecoversAfterConsistentPositions)
{
  GpsSpoofingDetector detector;
  detector.OnNetworkPosition(kKyiv, 70, 0);
  TEST(!detector.CheckSatellitePosition(kLima, 1000), ());

  int64_t time = 2000;
  for (int i = 1; i < GpsSpoofingDetector::kConsistentPositionsToRecover; ++i)
  {
    TEST(!detector.CheckSatellitePosition(kKyiv, time), ());
    time += 1000;
  }
  TEST(detector.CheckSatellitePosition(kKyiv, time), ());
  TEST(!detector.IsSpoofed(), ());
}

UNIT_TEST(NoGps_Spoofing_FlappingJammerResetsRecovery)
{
  GpsSpoofingDetector detector;
  detector.OnNetworkPosition(kKyiv, 70, 0);
  TEST(!detector.CheckSatellitePosition(kLima, 1000), ());

  int64_t time = 2000;
  for (int i = 1; i < GpsSpoofingDetector::kConsistentPositionsToRecover; ++i)
    TEST(!detector.CheckSatellitePosition(kKyiv, time += 1000), ());
  TEST(!detector.CheckSatellitePosition(kLima, time += 1000), ());
  TEST(!detector.CheckSatellitePosition(kKyiv, time += 1000), ());
  TEST(detector.IsSpoofed(), ());
}

UNIT_TEST(NoGps_Spoofing_RecoversSlowerWithoutNetwork)
{
  GpsSpoofingDetector detector;
  TEST(detector.CheckSatellitePosition(kKyiv, 0), ());
  TEST(!detector.CheckSatellitePosition(kLima, 1000), ());

  int64_t time = 1000;
  for (int i = 1; i < GpsSpoofingDetector::kConsistentPositionsToRecoverWithoutNetwork; ++i)
    TEST(!detector.CheckSatellitePosition(kKyiv, time += 1000), ());
  TEST(detector.CheckSatellitePosition(kKyiv, time += 1000), ());
}

// ~11 m per 0.0001 degree of latitude.
double constexpr kSmallStep = 0.0001;
using Own = GpsReturnDetector::Own;

// Feeds a GPS position every second for |seconds|, the car goes north at ~11 m/s from the own position.
bool Drive(GpsReturnDetector & detector, int64_t startMs, int seconds, double offsetLat,
           std::optional<double> ownErrorM)
{
  bool back = false;
  for (int i = 0; i <= seconds; ++i)
  {
    auto const car = North(kKyiv, i * kSmallStep);
    std::optional<Own> own;
    if (ownErrorM)
      own = Own{car, *ownErrorM};
    back = detector.OnGpsPosition(North(car, offsetLat), 10, startMs + i * 1000LL, own, {}, {}, false);
  }
  return back;
}

UNIT_TEST(NoGps_GpsReturn_TrustsGpsAfterStreak)
{
  GpsReturnDetector detector;
  TEST(!Drive(detector, 0, 14, 0, 20), ());
  auto const car = North(kKyiv, 15 * kSmallStep);
  TEST(detector.OnGpsPosition(car, 10, 15'000, Own{car, 20}, {}, {}, false), ());
}

UNIT_TEST(NoGps_GpsReturn_TrustsGpsWithoutOwnPosition)
{
  GpsReturnDetector detector;
  TEST(Drive(detector, 0, 15, 0, {}), ());
}

UNIT_TEST(NoGps_GpsReturn_DistrustsGpsFarFromOwnPosition)
{
  // A spoofer shifts the position by 500 m only, the cell towers would not notice it.
  GpsReturnDetector spoofed;
  TEST(!Drive(spoofed, 0, 60, 500 * kSmallStep / 11, 20), ());
  // An old mark: the car could have driven there.
  GpsReturnDetector oldMark;
  TEST(Drive(oldMark, 0, 15, 500 * kSmallStep / 11, 1000), ());
}

UNIT_TEST(NoGps_GpsReturn_RestartsStreakAfterGap)
{
  GpsReturnDetector detector;
  TEST(!Drive(detector, 0, 10, 0, 20), ());
  TEST(!Drive(detector, 15'000, 14, 0, 20), ());
  auto const car = North(kKyiv, 15 * kSmallStep);
  TEST(detector.OnGpsPosition(car, 10, 30'000, Own{car, 20}, {}, {}, false), ());
}

UNIT_TEST(NoGps_GpsReturn_RestartsStreakAfterJump)
{
  GpsReturnDetector detector;
  TEST(!Drive(detector, 0, 10, 0, {}), ());
  // 2 km in a second.
  TEST(!detector.OnGpsPosition(North(kKyiv, 0.02), 10, 11'000, {}, {}, {}, false), ());
  TEST(!detector.OnGpsPosition(North(kKyiv, 0.02), 10, 12'000, {}, {}, {}, false), ());
}

UNIT_TEST(NoGps_GpsReturn_DistrustsInaccurateGps)
{
  GpsReturnDetector detector;
  for (int i = 0; i <= 30; ++i)
    TEST(!detector.OnGpsPosition(kKyiv, 60, i * 1000LL, Own{kKyiv, 20}, {}, {}, false), ());
}

UNIT_TEST(NoGps_GpsReturn_IgnoresInaccurateGpsBetweenAccurate)
{
  // GPS and fused positions come together, the fused ones are rough.
  GpsReturnDetector detector;
  bool back = false;
  for (int i = 0; i <= 15; ++i)
  {
    TEST(!detector.OnGpsPosition(kKyiv, 60, i * 1000LL, Own{kKyiv, 20}, {}, {}, false), ());
    back = detector.OnGpsPosition(kKyiv, 10, i * 1000LL, Own{kKyiv, 20}, {}, {}, false);
  }
  TEST(back, ());
}

UNIT_TEST(NoGps_GpsReturn_RestartsStreakWhenOnlyInaccurateGps)
{
  GpsReturnDetector detector;
  TEST(!Drive(detector, 0, 10, 0, {}), ());
  for (int i = 11; i <= 20; ++i)
    TEST(!detector.OnGpsPosition(North(kKyiv, 10 * kSmallStep), 60, i * 1000LL, {}, {}, {}, false), ());
  TEST(!Drive(detector, 21'000, 10, 0, {}), ());
}

// Feeds GPS positions 500 m away from a wrong mark while the car goes north at ~11 m/s.
bool DriveAwayFromMark(GpsReturnDetector & detector, int seconds, std::optional<double> carSpeedMps, bool onRoad)
{
  bool back = false;
  for (int i = 0; i <= seconds; ++i)
  {
    auto const car = North(kKyiv, i * kSmallStep);
    back = detector.OnGpsPosition(North(car, 500 * kSmallStep / 11), 10, i * 1000LL, Own{car, 20}, 11.0, carSpeedMps,
                                  onRoad);
  }
  return back;
}

UNIT_TEST(NoGps_GpsReturn_TrustsGpsFollowingCar)
{
  // The mark is wrong, GPS is right: it goes along the roads as fast as the car.
  GpsReturnDetector detector;
  TEST(!DriveAwayFromMark(detector, 29, 11.5, true), ());
  auto const car = North(kKyiv, 30 * kSmallStep);
  TEST(detector.OnGpsPosition(North(car, 500 * kSmallStep / 11), 10, 30'000, Own{car, 20}, 11.0, 11.5, true), ());
}

UNIT_TEST(NoGps_GpsReturn_TrustsGpsFollowingCarInJam)
{
  // The car creeps at ~2 m/s: 120 m in a minute.
  GpsReturnDetector detector;
  bool back = false;
  for (int i = 0; i <= 60 && !back; ++i)
  {
    auto const car = North(kKyiv, i * kSmallStep * 2 / 11);
    back = detector.OnGpsPosition(North(car, 500 * kSmallStep / 11), 10, i * 1000LL, Own{car, 20}, 2.0, 2.2, true);
    TEST(back || i < 50, (i));
  }
  TEST(back, ());
}

UNIT_TEST(NoGps_GpsReturn_DistrustsGpsNotFollowingCar)
{
  // A spoofer moves the position while the car stands or drives slower.
  GpsReturnDetector standing, slower, unknown, offRoad;
  TEST(!DriveAwayFromMark(standing, 60, 0.0, true), ());
  TEST(!DriveAwayFromMark(slower, 60, 5.0, true), ());
  TEST(!DriveAwayFromMark(unknown, 60, {}, true), ());
  // It leads the position off the roads.
  TEST(!DriveAwayFromMark(offRoad, 60, 11.0, false), ());
}

UNIT_TEST(NoGps_GpsReturn_RestartsStreakAfterReset)
{
  GpsReturnDetector detector;
  TEST(!Drive(detector, 0, 10, 0, 20), ());
  detector.Reset();
  TEST(!Drive(detector, 11'000, 10, 0, 20), ());
}
}  // namespace nogps_gps_detectors_tests
