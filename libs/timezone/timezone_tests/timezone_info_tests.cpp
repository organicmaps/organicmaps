#include "testing/testing.hpp"

#include "timezone/timezone.hpp"

using namespace om::tz;

UNIT_TEST(TimeZoneImport_ShouldSuccessfullyImportTimeZoneDb)
{
  TimeZoneDb const & db = TimeZoneDb::Instance();
  TEST(!db.IsEmpty(), ());
}
