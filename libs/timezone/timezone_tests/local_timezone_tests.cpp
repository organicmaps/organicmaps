#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "timezone/local_timezone.hpp"

using namespace om::tz;

TEST(TimeZoneLocal, ShouldCreateLocalTimeZone)
{
  TimeZone const localTz = GetLocalTimeZone();

  // Just check that base offset is in reasonable range
  EXPECT_GE(localTz.base_offset, -12 * 60);
  EXPECT_LE(localTz.base_offset, 14 * 60);

  time_t const now = std::time(nullptr);
  std::tm local{};
  localtime_r(&now, &local);
  if (local.tm_isdst > 0)
  {
    EXPECT_THAT(localTz.dst_delta, testing::Not(testing::Eq(0)));
    EXPECT_THAT(localTz.transitions, testing::Not(testing::IsEmpty()));
  }
  else
  {
    EXPECT_THAT(localTz.dst_delta, testing::Eq(0));
    EXPECT_THAT(localTz.transitions, testing::IsEmpty());
  }
}
