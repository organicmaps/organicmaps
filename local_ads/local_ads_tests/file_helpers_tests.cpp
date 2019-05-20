#include "testing/testing.hpp"

#include "local_ads/file_helpers.hpp"

#include "base/file_name_utils.hpp"

#include "platform/platform_tests_support/scoped_file.hpp"

using namespace local_ads;
using namespace std;
using platform::tests_support::ScopedFile;

UNIT_TEST(LocalAdsHelpers_Read_Write_Country_Name)
{
  ScopedFile testFile("la_tests.dat", ScopedFile::Mode::Create);

  string const countryName = "Russia_Moscow";
  {
    FileWriter writer(testFile.GetFullPath());
    WriteCountryName(writer, countryName);
  }

  string result;
  {
    FileReader reader(testFile.GetFullPath());
    ReaderSource<FileReader> src(reader);
    result = ReadCountryName(src);
  }

  TEST_EQUAL(result, countryName, ());
}

UNIT_TEST(LocalAdsHelpers_Read_Write_Timestamp)
{
  ScopedFile testFile("la_tests.dat", ScopedFile::Mode::Create);

  auto ts = local_ads::Clock::now();
  {
    FileWriter writer(testFile.GetFullPath());
    WriteTimestamp<chrono::hours>(writer, ts);
    WriteTimestamp<chrono::seconds>(writer, ts);
  }

  local_ads::Timestamp resultInHours;
  local_ads::Timestamp resultInSeconds;
  {
    FileReader reader(testFile.GetFullPath());
    ReaderSource<FileReader> src(reader);
    resultInHours = ReadTimestamp<chrono::hours>(src);
    resultInSeconds = ReadTimestamp<chrono::seconds>(src);
  }

  TEST_EQUAL(chrono::duration_cast<chrono::hours>(ts - resultInHours).count(), 0, ());
  TEST_EQUAL(chrono::duration_cast<chrono::seconds>(ts - resultInSeconds).count(), 0, ());
}

UNIT_TEST(LocalAdsHelpers_Read_Write_RawData)
{
  ScopedFile testFile("la_tests.dat", ScopedFile::Mode::Create);

  vector<uint8_t> rawData = {1, 2, 3, 4, 5};
  {
    FileWriter writer(testFile.GetFullPath());
    WriteRawData(writer, rawData);
  }

  vector<uint8_t> result;
  {
    FileReader reader(testFile.GetFullPath());
    ReaderSource<FileReader> src(reader);
    result = ReadRawData(src);
  }

  TEST_EQUAL(rawData, result, ());
}
