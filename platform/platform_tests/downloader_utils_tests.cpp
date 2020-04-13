#include "testing/testing.hpp"

#include "platform/downloader_utils.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/mwm_version.hpp"

UNIT_TEST(UrlConvertionTest)
{
  {
    std::string const mwmName = "Luna";
    int64_t const dataVersion = version::FOR_TESTING_MWM1;
    int64_t const diffVersion = 0;
    MapFileType const fileType = MapFileType::Map;

    auto const path =
        platform::GetFileDownloadPath(dataVersion, platform::CountryFile(mwmName), fileType);

    auto const url = downloader::GetFileDownloadUrl(mwmName, dataVersion, diffVersion);
    auto const resultPath = downloader::GetFilePathByUrl(url);

    TEST_EQUAL(path, resultPath, ());
  }
  {
    std::string const mwmName = "Luna";
    int64_t const dataVersion = version::FOR_TESTING_MWM2;
    int64_t const diffVersion = version::FOR_TESTING_MWM1;
    MapFileType const fileType = MapFileType::Diff;

    auto const path =
        platform::GetFileDownloadPath(dataVersion, platform::CountryFile(mwmName), fileType);

    auto const url = downloader::GetFileDownloadUrl(mwmName, dataVersion, diffVersion);
    auto const resultPath = downloader::GetFilePathByUrl(url);

    TEST_EQUAL(path, resultPath, ());
  }
}
