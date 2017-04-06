#include "testing/testing.hpp"

#include "local_ads/local_ads_helpers.hpp"

#include "coding/file_name_utils.hpp"

#include "platform/platform_tests_support/scoped_file.hpp"

UNIT_TEST(LocalAdsHelpers_Read_Write_Country_Name)
{
  platform::tests_support::ScopedFile testFile("la_tests.dat");

  std::string const countryName = "Russia_Moscow";
  {
    FileWriter writer(testFile.GetFullPath());
    local_ads::WriteCountryName(writer, countryName);
  }

  std::string result;
  {
    FileReader reader(testFile.GetFullPath());
    ReaderSource<FileReader> src(reader);
    result = local_ads::ReadCountryName(src);
  }

  TEST_EQUAL(result, countryName, ());
}

UNIT_TEST(LocalAdsHelpers_Read_Write_Timestamp)
{
  platform::tests_support::ScopedFile testFile("la_tests.dat");

  auto ts = std::chrono::steady_clock::now();
  {
    FileWriter writer(testFile.GetFullPath());
    local_ads::WriteTimestamp<std::chrono::hours>(writer, ts);
    local_ads::WriteTimestamp<std::chrono::seconds>(writer, ts);
  }

  std::chrono::steady_clock::time_point resultInHours;
  std::chrono::steady_clock::time_point resultInSeconds;
  {
    FileReader reader(testFile.GetFullPath());
    ReaderSource<FileReader> src(reader);
    resultInHours = local_ads::ReadTimestamp<std::chrono::hours>(src);
    resultInSeconds = local_ads::ReadTimestamp<std::chrono::seconds>(src);
  }

  TEST_EQUAL(std::chrono::duration_cast<std::chrono::hours>(ts - resultInHours).count(), 0, ());
  TEST_EQUAL(std::chrono::duration_cast<std::chrono::seconds>(ts - resultInSeconds).count(), 0, ());
}

UNIT_TEST(LocalAdsHelpers_Read_Write_RawData)
{
  platform::tests_support::ScopedFile testFile("la_tests.dat");

  std::vector<uint8_t> rawData = {1, 2, 3, 4, 5};
  {
    FileWriter writer(testFile.GetFullPath());
    local_ads::WriteRawData(writer, rawData);
  }

  std::vector<uint8_t> result;
  {
    FileReader reader(testFile.GetFullPath());
    ReaderSource<FileReader> src(reader);
    result = local_ads::ReadRawData(src);
  }

  TEST_EQUAL(rawData, result, ());
}
