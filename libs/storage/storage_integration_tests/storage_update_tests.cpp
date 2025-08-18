#include "testing/testing.hpp"

#include "storage/storage_integration_tests/test_defines.hpp"

#include "map/framework.hpp"

#include "platform/downloader_defines.hpp"
#include "platform/http_request.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/writable_dir_changer.hpp"

#include "coding/internal/file_data.hpp"

#include "storage/storage.hpp"

#include "base/file_name_utils.hpp"

#include <memory>
#include <string>

namespace storage_update_tests
{
using namespace platform;
using namespace std;
using namespace storage;

static FrameworkParams const kFrameworkParams(false /* m_enableDiffs */);

string const kCountriesTxtFile = COUNTRIES_FILE;

string const kMwmVersion1 = "190830";
// size_t const kCountriesTxtFileSize1 = 420632;

string const kMwmVersion2 = "190910";
// size_t const kCountriesTxtFileSize2 = 420634;

string const kGroupCountryId = "Belarus";

bool DownloadFile(string const & url, string const & filePath, size_t fileSize)
{
  using namespace downloader;

  DownloadStatus httpStatus;
  bool finished = false;

  unique_ptr<HttpRequest> request(HttpRequest::GetFile({url}, filePath, fileSize, [&](HttpRequest & request)
  {
    DownloadStatus const s = request.GetStatus();
    if (s != DownloadStatus::InProgress)
    {
      httpStatus = s;
      finished = true;
      testing::StopEventLoop();
    }
  }));

  testing::RunEventLoop();

  return httpStatus == DownloadStatus::Completed;
}

string GetCountriesTxtWebUrl(string const version)
{
  return kTestWebServer + "direct/" + version + "/" + kCountriesTxtFile;
}

string GetCountriesTxtFilePath()
{
  return base::JoinPath(GetPlatform().WritableDir(), kCountriesTxtFile);
}

string GetMwmFilePath(string const & version, CountryId const & countryId)
{
  return base::JoinPath(GetPlatform().WritableDir(), version, countryId + DATA_FILE_EXTENSION);
}

/// @todo We don't have direct version links for now.
/// Also Framework f(kFrameworkParams) will fail here, @see SmallMwms_3levels_Test.
/*
UNIT_TEST(SmallMwms_Update_Test)
{
  WritableDirChanger writableDirChanger(kMapTestDir);

  Platform & platform = GetPlatform();

  auto onProgressFn = [&](CountryId const &, downloader::Progress const &) {};

  // Download countries.txt for version 1
  TEST(DownloadFile(GetCountriesTxtWebUrl(kMwmVersion1), GetCountriesTxtFilePath(), kCountriesTxtFileSize1), ());

  {
    Framework f(kFrameworkParams);
    auto & storage = f.GetStorage();
    string const version = strings::to_string(storage.GetCurrentDataVersion());
    TEST_EQUAL(version, kMwmVersion1, ());
    auto onChangeCountryFn = [&](CountryId const & countryId) {
      if (!storage.IsDownloadInProgress())
        testing::StopEventLoop();
    };
    storage.Subscribe(onChangeCountryFn, onProgressFn);
    storage.SetDownloadingServersForTesting({kTestWebServer});

    CountriesVec children;
    storage.GetChildren(kGroupCountryId, children);

    // Download group
    storage.DownloadNode(kGroupCountryId);
    testing::RunEventLoop();

    // Check group node status is OnDisk
    NodeAttrs attrs;
    storage.GetNodeAttrs(kGroupCountryId, attrs);
    TEST_EQUAL(NodeStatus::OnDisk, attrs.m_status, ());

    // Check mwm files for version 1 are present
    for (auto const & child : children)
    {
      string const mwmFullPathV1 = GetMwmFilePath(kMwmVersion1, child);
      TEST(platform.IsFileExistsByFullPath(mwmFullPathV1), ());
    }
  }

  // Replace countries.txt by version 2
  TEST(base::DeleteFileX(GetCountriesTxtFilePath()), ());
  TEST(DownloadFile(GetCountriesTxtWebUrl(kMwmVersion2), GetCountriesTxtFilePath(), kCountriesTxtFileSize2), ());

  {
    Framework f(kFrameworkParams);
    auto & storage = f.GetStorage();
    string const version = strings::to_string(storage.GetCurrentDataVersion());
    TEST_EQUAL(version, kMwmVersion2, ());
    auto onChangeCountryFn = [&](CountryId const & countryId) {
      if (!storage.IsDownloadInProgress())
        testing::StopEventLoop();
    };
    storage.Subscribe(onChangeCountryFn, onProgressFn);
    storage.SetDownloadingServersForTesting({kTestWebServer});

    CountriesVec children;
    storage.GetChildren(kGroupCountryId, children);

    // Check group node status is OnDiskOutOfDate
    NodeAttrs attrs;
    storage.GetNodeAttrs(kGroupCountryId, attrs);
    TEST_EQUAL(NodeStatus::OnDiskOutOfDate, attrs.m_status, ());

    // Check children node status is OnDiskOutOfDate
    for (auto const & child : children)
    {
      NodeAttrs attrs;
      storage.GetNodeAttrs(child, attrs);
      TEST_EQUAL(NodeStatus::OnDiskOutOfDate, attrs.m_status, ());
    }

    // Check mwm files for version 1 are present
    for (auto const & child : children)
    {
      string const mwmFullPathV1 = GetMwmFilePath(kMwmVersion1, child);
      TEST(platform.IsFileExistsByFullPath(mwmFullPathV1), ());
    }

    // Download group, new version
    storage.DownloadNode(kGroupCountryId);
    testing::RunEventLoop();

    // Check group node status is OnDisk
    storage.GetNodeAttrs(kGroupCountryId, attrs);
    TEST_EQUAL(NodeStatus::OnDisk, attrs.m_status, ());

    // Check children node status is OnDisk
    for (auto const & child : children)
    {
      NodeAttrs attrs;
      storage.GetNodeAttrs(child, attrs);
      TEST_EQUAL(NodeStatus::OnDisk, attrs.m_status, ());
    }

    // Check mwm files for version 2 are present and not present for version 1
    for (auto const & child : children)
    {
      string const mwmFullPathV1 = GetMwmFilePath(kMwmVersion1, child);
      string const mwmFullPathV2 = GetMwmFilePath(kMwmVersion2, child);
      TEST(platform.IsFileExistsByFullPath(mwmFullPathV2), ());
      TEST(!platform.IsFileExistsByFullPath(mwmFullPathV1), ());
    }
  }
}
*/
}  // namespace storage_update_tests
