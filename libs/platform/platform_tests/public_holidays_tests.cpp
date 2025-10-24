#include "testing/testing.hpp"

#include "platform/public_holidays.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"


#include <ctime>
#include <string>


namespace public_holidays_tests {
  using platform::tests_support::ScopedDir;
  using platform::tests_support::ScopedFile;

  namespace
  {
    char const * NONEXISTENT_COUNTRY = "NONEXISTENT";

    time_t CreateTime(int year, int month, int day)
    {
      std::tm tm = {};
      tm.tm_year = year - 1900;
      tm.tm_mon = month - 1;
      tm.tm_mday = day;
      tm.tm_hour = 12;
      tm.tm_min = 0;
      tm.tm_sec = 0;
      return std::mktime(&tm);
    }

    // Define synthetic test data
    char const kTestHolidayJson[] = R"({
  "2025-01-01": "New Year's Day",
  "2025-05-29": "Ascension Day",
  "2025-08-01": "National Day",
  "2025-12-25": "Christmas Day",
  "2026-01-01": "New Year's Day",
  "2026-05-14": "Ascension Day",
  "2026-08-01": "National Day",
  "2026-12-25": "Christmas Day"
})";
  }  // namespace

  UNIT_TEST(TimeTToISODate_ValidDates)
  {
    // Test New Year's Day 2024
    time_t newYear2024 = CreateTime(2024, 1, 1);
    TEST_EQUAL(ph::TimeTToISODate(newYear2024), "2024-01-01", ());

    // Test Christmas 2024
    time_t christmas2024 = CreateTime(2024, 12, 25);
    TEST_EQUAL(ph::TimeTToISODate(christmas2024), "2024-12-25", ());

    // Test leap year date
    time_t leapYearDate = CreateTime(2024, 2, 29);
    TEST_EQUAL(ph::TimeTToISODate(leapYearDate), "2024-02-29", ());

    // Test single digit month/day
    time_t singleDigitDate = CreateTime(2024, 3, 5);
    TEST_EQUAL(ph::TimeTToISODate(singleDigitDate), "2024-03-05", ());
  }

  UNIT_TEST(HasHolidays_FileExists)
  {
    // Create synthetic test file
    ScopedFile const holidayFile("countries/public_holidays/TestCountry.json", kTestHolidayJson);

    // Test that the file is found
    TEST(ph::HasHolidays("TestCountry"), ());

    // Test non-existent country (should always fail)
    TEST(!ph::HasHolidays(NONEXISTENT_COUNTRY), ());
  }

  UNIT_TEST(LoadCountryHolidays_ValidJSON)
  {
    // Create synthetic test file
    ScopedFile const holidayFile("countries/public_holidays/TestCountry.json", kTestHolidayJson);

    auto holidays = ph::LoadCountryHolidays("TestCountry");

    // Check that holidays were loaded (our test has 8 holidays)
    TEST_EQUAL(holidays.size(), 8, ());

    // Check specific known holidays from our test JSON
    TEST_EQUAL(holidays["2025-01-01"], "New Year's Day", ());
    TEST_EQUAL(holidays["2025-05-29"], "Ascension Day", ());
    TEST_EQUAL(holidays["2025-08-01"], "National Day", ());
    TEST_EQUAL(holidays["2025-12-25"], "Christmas Day", ());
    TEST_EQUAL(holidays["2026-01-01"], "New Year's Day", ());
  }

  UNIT_TEST(LoadCountryHolidays_FileNotExists)
  {
    auto holidays = ph::LoadCountryHolidays(NONEXISTENT_COUNTRY);

    // Should return empty map for non-existent file
    TEST_EQUAL(holidays.size(), 0, ());
  }

  UNIT_TEST(GetHolidayName_ValidHoliday)
  {
    // Create synthetic test file
    ScopedFile const holidayFile("countries/public_holidays/TestCountry.json", kTestHolidayJson);

    // Test New Year's Day 2025
    time_t newYear2025 = CreateTime(2025, 1, 1);
    std::string holidayName;
    TEST(ph::GetHolidayName("TestCountry", newYear2025, holidayName), ());
    TEST_EQUAL(holidayName, "New Year's Day", ());

    // Test Ascension Day 2025
    time_t ascensionDay2025 = CreateTime(2025, 5, 29);
    TEST(ph::GetHolidayName("TestCountry", ascensionDay2025, holidayName), ());
    TEST_EQUAL(holidayName, "Ascension Day", ());

    // Test National Day 2025
    time_t nationalDay2025 = CreateTime(2025, 8, 1);
    TEST(ph::GetHolidayName("TestCountry", nationalDay2025, holidayName), ());
    TEST_EQUAL(holidayName, "National Day", ());

    // Test Christmas 2025
    time_t christmas2025 = CreateTime(2025, 12, 25);
    TEST(ph::GetHolidayName("TestCountry", christmas2025, holidayName), ());
    TEST_EQUAL(holidayName, "Christmas Day", ());
  }

  UNIT_TEST(GetHolidayName_NoHoliday)
  {
    // Create synthetic test file
    ScopedFile const holidayFile("countries/public_holidays/TestCountry.json", kTestHolidayJson);

    // Test date that's not a holiday (June 15, 2025)
    time_t regularDate = CreateTime(2025, 6, 15);
    std::string holidayName;
    TEST(!ph::GetHolidayName("TestCountry", regularDate, holidayName), ());
    TEST_EQUAL(holidayName, "", ());
  }

  UNIT_TEST(GetHolidayName_CountryNotExists)
  {
    time_t testDate = CreateTime(2025, 1, 1);
    std::string holidayName;
    TEST(!ph::GetHolidayName(NONEXISTENT_COUNTRY, testDate, holidayName), ());
    TEST_EQUAL(holidayName, "", ());
  }

  UNIT_TEST(LoadHolidaysDate_ValidCountry)
  {
    // Create synthetic test file
    ScopedFile const holidayFile("countries/public_holidays/TestCountry.json", kTestHolidayJson);

    auto holidayDates = ph::LoadHolidaysDate("TestCountry");

    // Verify we loaded all 8 holidays from kTestHolidayJson
    TEST_EQUAL(holidayDates.size(), 8, ());

    // Verify some specific dates by converting back to ISO format
    bool found2025NewYear = false;
    bool found2025Christmas = false;
    bool found2026NewYear = false;

    for (auto const & date : holidayDates)
    {
      std::string isoDate = ph::TimeTToISODate(date);
      if (isoDate == "2025-01-01")
        found2025NewYear = true;
      if (isoDate == "2025-12-25")
        found2025Christmas = true;
      if (isoDate == "2026-01-01")
        found2026NewYear = true;
    }

    TEST(found2025NewYear, ());
    TEST(found2025Christmas, ());
    TEST(found2026NewYear, ());
  }

  UNIT_TEST(LoadHolidaysDate_InvalidDateFormat)
  {
    char const kInvalidDateJson[] = R"({
  "2025-01-01": "Valid Holiday",
  "INVALID-DATE": "Bad Format Holiday",
  "2025-12-25": "Another Valid Holiday"
})";
    ScopedFile const holidayFile("countries/public_holidays/InvalidCountry.json", kInvalidDateJson);

    auto holidayDates = ph::LoadHolidaysDate("InvalidCountry");

    // Should only load the 2 valid dates, skipping the invalid one
    TEST_EQUAL(holidayDates.size(), 2, ());

    // Verify the valid dates are present
    bool foundNewYear = false;
    bool foundChristmas = false;

    for (auto const & date : holidayDates)
    {
      std::string isoDate = ph::TimeTToISODate(date);
      if (isoDate == "2025-01-01")
        foundNewYear = true;
      if (isoDate == "2025-12-25")
        foundChristmas = true;
    }

    TEST(foundNewYear, ());
    TEST(foundChristmas, ());
  }
  }  // namespace public_holidays_tests
