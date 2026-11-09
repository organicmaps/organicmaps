#include "testing/testing.hpp"

#include "platform/location.hpp"

UNIT_TEST(IsLatValid)
{
  TEST(location::IsLatValid(35.), ());
  TEST(location::IsLatValid(-35.), ());
  TEST(!location::IsLatValid(0.), ());
  TEST(!location::IsLatValid(100.), ());
  TEST(!location::IsLatValid(-99.), ());
}

UNIT_TEST(IsLonValid)
{
  TEST(location::IsLonValid(135.), ());
  TEST(location::IsLonValid(-35.), ());
  TEST(!location::IsLonValid(0.), ());
  TEST(!location::IsLonValid(200.), ());
  TEST(!location::IsLonValid(-199.), ());
}

UNIT_TEST(AngleToBearing)
{
  TEST_ALMOST_EQUAL_ULPS(location::AngleToBearing(0.), 90., ());
  TEST_ALMOST_EQUAL_ULPS(location::AngleToBearing(30.), 60., ());
  TEST_ALMOST_EQUAL_ULPS(location::AngleToBearing(100.), 350., ());
  TEST_ALMOST_EQUAL_ULPS(location::AngleToBearing(370.), 80., ());
  TEST_ALMOST_EQUAL_ULPS(location::AngleToBearing(-370.), 100., ());
}

UNIT_TEST(BearingToAngle)
{
  TEST_ALMOST_EQUAL_ULPS(location::BearingToAngle(0.), 90., ());
  TEST_ALMOST_EQUAL_ULPS(location::BearingToAngle(30.), 60., ());
  TEST_ALMOST_EQUAL_ULPS(location::BearingToAngle(100.), 350., ());
  TEST_ALMOST_EQUAL_ULPS(location::BearingToAngle(370.), 80., ());
  TEST_ALMOST_EQUAL_ULPS(location::BearingToAngle(-370.), 100., ());
}

UNIT_TEST(TLocationError_DebugPrint)
{
  TEST_EQUAL(DebugPrint(location::ENotSupported), "ENotSupported", ());
  TEST_EQUAL(DebugPrint(location::EGPSIsOff), "EGPSIsOff", ());
}

UNIT_TEST(LocationTiming_FreshnessUsesTheObservationClock)
{
  using namespace std::chrono_literals;
  location::GpsInfo fix;
  auto const observed = location::Clock::time_point(100s);
  fix.m_timing = location::GpsInfo::Timing{observed, observed + 5s};

  TEST(!fix.IsFresh(observed - 1ns), ());
  TEST(fix.IsFresh(observed), ());
  TEST(fix.IsFresh(observed + 4999ms), ());
  TEST(!fix.IsFresh(observed + 5s), ());
  TEST(fix.m_timing->m_observedAt == observed, ());
}

UNIT_TEST(LocationTiming_LegacyInputsKeepTheirClockDomain)
{
  location::GpsInfo legacy;
  legacy.m_timestamp = 12345;
  TEST(legacy.IsFresh(), ());

  auto timed = legacy;
  timed.m_timing = location::GpsInfo::Timing{location::Clock::time_point(std::chrono::seconds(1)),
                                             location::Clock::time_point(std::chrono::seconds(6))};
  TEST(!legacy.HasSameClock(timed), ());
  TEST(timed.m_timing->m_observedAt == location::Clock::time_point(std::chrono::seconds(1)), ());
  TEST_EQUAL(timed.m_timestamp, legacy.m_timestamp, ());
}
