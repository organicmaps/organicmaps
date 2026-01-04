#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "timezone/local_timezone.hpp"

using namespace om::tz;

TEST(TimeZoneLocal, ShouldCreateLocalTimeZone)
{
  LocalTimeZone const localTz = GetLocalTimeZone();

  // Just check that base offset is in reasonable range
  EXPECT_GE(localTz.base_offset, -12 * 60);
  EXPECT_LE(localTz.base_offset, 14 * 60);

  time_t const now = std::time(nullptr);
  std::tm local{};
  localtime_r(&now, &local);
  EXPECT_EQ(local.tm_isdst, localTz.dst_delta);
  if (localTz.dst_delta > 0)
    EXPECT_THAT(localTz.dst_delta, testing::Not(testing::Eq(0)));
  else
    EXPECT_THAT(localTz.dst_delta, testing::Eq(0));
}
