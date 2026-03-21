#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "timezone/timezone.hpp"

TEST(TimeZoneImport, ShouldSuccessfullyImportTimeZoneDb)
{
  om::tz::TimeZoneDb const & db = om::tz::TimeZoneDb::Instance();
  EXPECT_FALSE(db.IsEmpty());
}
