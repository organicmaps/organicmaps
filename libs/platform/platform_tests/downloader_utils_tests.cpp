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
             base::JoinPath(GetPlatform().WritableDir(), "220314", "Belarus_Brest Region.mwm.ready"), ());

  {
    // The terrain blocks: terrain/<version>/<name>.twm -> <writable>/terrain/<version>/<name>.twm.ready.
    auto const url = downloader::GetTerrainDownloadUrl(260728, "N40E040.twm");
    TEST_EQUAL(url, "terrain/260728/N40E040.twm", ());
    TEST_EQUAL(downloader::GetFilePathByUrl(url),
               base::JoinPath(GetPlatform().WritableDir(), "terrain/260728/N40E040.twm.ready"), ());
  }
}

UNIT_TEST(Downloader_TerrainDataDir)
{
  auto const path = base::JoinPath(GetPlatform().WritableDir(), "custom", "terrain", "260728", "N40E040.twm");
  TEST_EQUAL(platform::GetFilePath(260728, "custom", "N40E040", MapFileType::Terrain), path, ());
  TEST_EQUAL(platform::GetFileDownloadPath(260728, "custom", "N40E040", MapFileType::Terrain), path + ".ready", ());
  TEST_EQUAL(platform::GetFileDownloadPath(260728, "N40E040", MapFileType::Terrain),
             downloader::GetFilePathByUrl("terrain/260728/N40E040.twm"), ());
  TEST_EQUAL(platform::GetFilePath(0, "custom", "N40E040", MapFileType::Terrain),
             base::JoinPath(GetPlatform().WritableDir(), "custom", "terrain", "0", "N40E040.twm"), ());
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

  TEST(downloader::IsUrlSupported("terrain/260728/N40E040.twm"), ());
  TEST(!downloader::IsUrlSupported("terrain/abc/N40E040.twm"), ());
  TEST(!downloader::IsUrlSupported("terrain/-1/N40E040.twm"), ());
  TEST(!downloader::IsUrlSupported("terrain/260728/N40E040.mwm"), ());
  TEST(!downloader::IsUrlSupported("terrain/N40E040.twm"), ());
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

UNIT_TEST(Downloader_RejectsInvalidFileNames)
{
  for (auto const & path : {"maps/260728/", "diffs/260728/260727/", "terrain/260728/"})
  {
    auto const extension = std::string(path).starts_with("terrain/") ? ".twm"
                         : std::string(path).starts_with("diffs/")   ? ".mwmdiff"
                                                                     : ".mwm";
    for (auto const & name : {"../N40E040", "%2E%2E%2FN40E040", "%2fN40E040", "N40E040%2FN40E041", "N40E040%5CN40E041",
                              "N40E040%00", "N40E040.extra", ".N40E040", "N40E040.", ""})
    {
      auto const url = std::string(path) + name + extension;
      TEST(!downloader::IsUrlSupported(url), (url));
    }

    auto const url = std::string(path) + "N40E040" + extension;
    TEST(downloader::IsUrlSupported(url), (url));
  }
  TEST(downloader::IsUrlSupported("terrain/260728/N40E040%2Etwm"), ());
}

UNIT_TEST(Downloader_ParseMetaConfig)
{
  auto cfg = downloader::ParseMetaConfig(R"({"servers":[ "https://url1/", "https://url2/" ]})");

  TEST(cfg, ());
  TEST_EQUAL(cfg->servers.size(), 2, ());
  TEST_EQUAL(cfg->servers[0], "https://url1/", ());
  TEST_EQUAL(cfg->servers[1], "https://url2/", ());

  cfg = downloader::ParseMetaConfig(R"(
    {
      "servers": [ "https://url1/", "https://url2/" ],
      "settings": {
        "DonateUrl": "value1",
        "NY": "value2",
        "key3": "value3"
      }
    }
  )");
  TEST(cfg, ());
  TEST_EQUAL(cfg->servers.size(), 2, ());
  TEST_EQUAL(cfg->servers[0], "https://url1/", ());
  TEST_EQUAL(cfg->servers[1], "https://url2/", ());
  TEST_EQUAL(cfg->settings.size(), 3, ());
  TEST_EQUAL(cfg->settings["DonateUrl"], "value1", ());
  TEST_EQUAL(cfg->settings["NY"], "value2", ());
  TEST_EQUAL(cfg->settings["key3"], "value3", ());

  TEST(!downloader::ParseMetaConfig("Broken JSON"), ());

  TEST(!downloader::ParseMetaConfig("[]"), ("Empty array"));

  TEST(!downloader::ParseMetaConfig("{}"), ("Empty object"));

  TEST(!downloader::ParseMetaConfig(R"({"no_servers": "invalid"})"), ());

  TEST(!downloader::ParseMetaConfig(R"({"servers": "invalid"})"), ());
}
