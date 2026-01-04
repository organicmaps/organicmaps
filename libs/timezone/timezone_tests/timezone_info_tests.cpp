#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "timezone/timezone.hpp"

using namespace om::tz;

TEST(TimeZoneImport, ShouldSuccessfullyImportTimeZoneDb)
{
  TimeZoneDb const & db = GetTimeZoneDb();

  EXPECT_THAT(db.timezones, testing::Not(testing::IsEmpty()));
}
