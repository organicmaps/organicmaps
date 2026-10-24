#include "testing/testing.hpp"

#include "platform/location_service/position_filter.hpp"

#include <ctime>

namespace position_filter_tests
{
constexpr std::time_t kNow = 1000;

UNIT_TEST(PositionFilter_RejectsUpdatesOlderThanLatestAcceptedFix)
{
  location::PositionFilter filter;
  location::GpsInfo info;
  info.m_timestamp = kNow - 100;
  TEST(filter.Passes(info, kNow), ());
  info.m_timestamp += 50;
  TEST(filter.Passes(info, kNow), ());
  info.m_timestamp -= 20;
  TEST(!filter.Passes(info, kNow), ());
  info.m_timestamp += 10;
  TEST(!filter.Passes(info, kNow), ("A rejected fix must not rewind the reference"));
  info.m_timestamp += 10;
  TEST(filter.Passes(info, kNow), ("Equal timestamps are allowed"));
}

UNIT_TEST(PositionFilter_ExpiredFixDoesNotRewindReference)
{
  location::PositionFilter filter;
  location::GpsInfo info;
  info.m_timestamp = kNow - 301;
  TEST(!filter.Passes(info, kNow), ());
  info.m_timestamp = kNow - 100;
  TEST(filter.Passes(info, kNow), ());
  info.m_timestamp = kNow - 301;
  TEST(!filter.Passes(info, kNow), ());
  info.m_timestamp = kNow - 200;
  TEST(!filter.Passes(info, kNow), ("An expired fix must not allow older unexpired fixes through"));
  info.m_timestamp = kNow - 50;
  TEST(filter.Passes(info, kNow), ());
}

UNIT_TEST(PositionFilter_FreshnessBoundary)
{
  for (double const age : {300.0, 300.001})
  {
    location::PositionFilter filter;
    location::GpsInfo info;
    info.m_timestamp = kNow - age;
    TEST_EQUAL(filter.Passes(info, kNow), age == 300.0, (age));
  }
}

UNIT_TEST(PositionFilter_BackwardClockCorrection)
{
  location::PositionFilter filter;
  location::GpsInfo info;
  info.m_timestamp = kNow;
  TEST(filter.Passes(info, kNow), ());
  info.m_timestamp = kNow + 3600;
  TEST(filter.Passes(info, kNow + 3600), ());

  info.m_timestamp = kNow;
  TEST(filter.Passes(info, kNow), ("A clock correction must not block subsequent fixes"));
  info.m_timestamp = kNow + 1;
  TEST(filter.Passes(info, kNow + 1), ());
  info.m_timestamp = kNow + 0.5;
  TEST(!filter.Passes(info, kNow + 1), ("Ordering resumes after the clock correction"));
}

UNIT_TEST(PositionFilter_ExpiredFixAfterClockCorrection)
{
  location::PositionFilter filter;
  location::GpsInfo info;
  info.m_timestamp = kNow + 3600;
  TEST(filter.Passes(info, kNow + 3600), ());
  info.m_timestamp = kNow - 301;
  TEST(!filter.Passes(info, kNow), ("A clock correction must not bypass freshness checks"));
  info.m_timestamp = kNow - 100;
  TEST(filter.Passes(info, kNow), ());
  info.m_timestamp -= 1;
  TEST(!filter.Passes(info, kNow), ());
}
}  // namespace position_filter_tests
