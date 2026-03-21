#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "timezone/timezone.hpp"

namespace timezone_info_tests
{
using namespace om::tz;

TEST(TimeZoneImport, ShouldSuccessfullyImportTimeZoneDb)
{
  TimeZoneDb const & db = TimeZoneDb::Instance();
  EXPECT_FALSE(db.IsEmpty());
}
}  // namespace timezone_info_tests
