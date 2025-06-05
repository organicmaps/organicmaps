#include "testing/testing.hpp"

#include "platform/downloader_utils.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"
#include "platform/servers_list.hpp"

#include "base/file_name_utils.hpp"

UNIT_TEST(Downloader_GetFilePathByUrl)
{
  {
    std::string const mwmName = "Luna";
    std::string const fileName = platform::GetFileName(mwmName, MapFileType::Map);
    int64_t const dataVersion = version::FOR_TESTING_MWM1;
    int64_t const diffVersion = 0;
    MapFileType const fileType = MapFileType::Map;

    auto const path = platform::GetFileDownloadPath(dataVersion, mwmName, fileType);

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

    auto const path = platform::GetFileDownloadPath(dataVersion, mwmName, fileType);

    auto const url = downloader::GetFileDownloadUrl(fileName, dataVersion, diffVersion);
    auto const resultPath = downloader::GetFilePathByUrl(url);

    TEST_EQUAL(path, resultPath, ());
  }

  TEST_EQUAL(downloader::GetFilePathByUrl("/maps/220314/Belarus_Brest Region.mwm"),
             base::JoinPath(GetPlatform().WritableDir(), "220314/Belarus_Brest Region.mwm.ready"), ());
}

UNIT_TEST(Downloader_IsUrlSupported)
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

UNIT_TEST(Downloader_ParseMetaConfig)
{
  std::optional<downloader::MetaConfig> cfg;

  TEST((cfg = downloader::ParseMetaConfig(R"([ "https://url1/", "https://url2/" ])")), ());
  TEST_EQUAL(cfg->m_serversList.size(), 2, ());
  TEST_EQUAL(cfg->m_serversList[0], "https://url1/", ());
  TEST_EQUAL(cfg->m_serversList[1], "https://url2/", ());

  TEST((cfg = downloader::ParseMetaConfig(R"(
    {
      "servers": [ "https://url1/", "https://url2/" ],
      "settings": {
        "DonateUrl": "value1",
        "NY": "value2",
        "key3": "value3"
      }
    }
  )")),
       ());
  TEST_EQUAL(cfg->m_serversList.size(), 2, ());
  TEST_EQUAL(cfg->m_serversList[0], "https://url1/", ());
  TEST_EQUAL(cfg->m_serversList[1], "https://url2/", ());
  TEST_EQUAL(cfg->m_settings.size(), 2, ());  // "key3" is ignored
  TEST_EQUAL(cfg->m_settings["DonateUrl"], "value1", ());
  TEST_EQUAL(cfg->m_settings["NY"], "value2", ());

  TEST(!downloader::ParseMetaConfig(R"(broken json)"), ());

  TEST(!downloader::ParseMetaConfig(R"([])"), ());

  TEST(!downloader::ParseMetaConfig(R"({})"), ());

  TEST(!downloader::ParseMetaConfig(R"({"no_servers": "invalid"})"), ());

  TEST(!downloader::ParseMetaConfig(R"({"servers": "invalid"})"), ());
}
