#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "timezone/timezone.hpp"

using namespace om::tz;

TEST(TimeZoneImport, ShouldSuccessfullyImportTimeZoneDb)
{
  TimeZoneDb const & db = TimeZoneDb::Instance();
  EXPECT_FALSE(db.IsEmpty());
}
