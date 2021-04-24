#include "testing/testing.hpp"

#include "platform/downloader_utils.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/mwm_version.hpp"

UNIT_TEST(UrlConversionTest)
{
  {
    std::string const mwmName = "Luna";
    std::string const fileName = platform::GetFileName(mwmName, MapFileType::Map);
    int64_t const dataVersion = version::FOR_TESTING_MWM1;
    int64_t const diffVersion = 0;
    MapFileType const fileType = MapFileType::Map;

    auto const path =
        platform::GetFileDownloadPath(dataVersion, platform::CountryFile(mwmName), fileType);

    auto const url = downloader::GetFileDownloadUrl(fileName, dataVersion, diffVersion);
    auto const resultPath = downloader::GetFilePathByUrl(url);

    TEST_EQUAL(path, resultPath, ());
  }
  {
    std::string const mwmName = "Luna";
    std::string const fileName = platform::GetFileName(mwmName, MapFileType::Diff);
    int64_t const dataVersion = version::FOR_TESTING_MWM2;
    int64_t const diffVersion = version::FOR_TESTING_MWM1;
    MapFileType const fileType = MapFileType::Diff;

    auto const path =
        platform::GetFileDownloadPath(dataVersion, platform::CountryFile(mwmName), fileType);

    auto const url = downloader::GetFileDownloadUrl(fileName, dataVersion, diffVersion);
    auto const resultPath = downloader::GetFilePathByUrl(url);

    TEST_EQUAL(path, resultPath, ());
  }
}

UNIT_TEST(IsUrlSupportedTest)
{
  std::string const mwmName = "Luna";

  std::string fileName = platform::GetFileName(mwmName, MapFileType::Map);
  int64_t dataVersion = version::FOR_TESTING_MWM1;
  int64_t diffVersion = 0;

  auto url = downloader::GetFileDownloadUrl(fileName, dataVersion, diffVersion);
  TEST(downloader::IsUrlSupported(url), ());
  TEST(downloader::IsUrlSupported("maps/991215/Luna.mwm"), ());
  TEST(downloader::IsUrlSupported("maps/0/Luna.mwm"), ());
  TEST(!downloader::IsUrlSupported("maps/x/Luna.mwm"), ());
  TEST(!downloader::IsUrlSupported("macarena/0/Luna.mwm"), ());
  TEST(!downloader::IsUrlSupported("/hack/maps/0/Luna.mwm"), ());
  TEST(!downloader::IsUrlSupported("0/Luna.mwm"), ());
  TEST(!downloader::IsUrlSupported("maps/0/Luna"), ());
  TEST(!downloader::IsUrlSupported("0/Luna.mwm"), ());
  TEST(!downloader::IsUrlSupported("Luna.mwm"), ());
  TEST(!downloader::IsUrlSupported("Luna"), ());

  fileName = platform::GetFileName(mwmName, MapFileType::Diff);
  diffVersion = version::FOR_TESTING_MWM1;
  url = downloader::GetFileDownloadUrl(fileName, dataVersion, diffVersion);
  TEST(downloader::IsUrlSupported(url), ());
  TEST(downloader::IsUrlSupported("diffs/991215/991215/Luna.mwmdiff"), ());
  TEST(downloader::IsUrlSupported("diffs/0/0/Luna.mwmdiff"), ());
  TEST(!downloader::IsUrlSupported("diffs/x/0/Luna.mwmdiff"), ());
  TEST(!downloader::IsUrlSupported("diffs/0/x/Luna.mwmdiff"), ());
  TEST(!downloader::IsUrlSupported("diffs/x/x/Luna.mwmdiff"), ());
  TEST(!downloader::IsUrlSupported("beefs/0/0/Luna.mwmdiff"), ());
  TEST(!downloader::IsUrlSupported("diffs/0/0/Luna.mwmdiff.f"), ());
  TEST(!downloader::IsUrlSupported("maps/diffs/0/0/Luna.mwmdiff"), ());
  TEST(!downloader::IsUrlSupported("diffs/0/0/Luna"), ());
  TEST(!downloader::IsUrlSupported("0/0/Luna.mwmdiff"), ());
  TEST(!downloader::IsUrlSupported("diffs/0/Luna.mwmdiff"), ());
  TEST(!downloader::IsUrlSupported("diffs/0"), ());
  TEST(!downloader::IsUrlSupported("diffs/0/Luna.mwmdiff"), ());
  TEST(!downloader::IsUrlSupported("diffs/Luna.mwmdiff"), ());
  TEST(!downloader::IsUrlSupported("Luna.mwmdiff"), ());
  TEST(!downloader::IsUrlSupported("Luna"), ());
}
