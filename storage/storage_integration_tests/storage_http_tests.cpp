#include "testing/testing.hpp"

#include "storage/storage.hpp"

#include "platform/local_country_file_utils.hpp"
#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/write_dir_changer.hpp"

#include "coding/file_name_utils.hpp"

#include "base/scope_guard.hpp"
#include "base/string_utils.hpp"
#include "base/thread.hpp"

#include "std/string.hpp"

using namespace platform;
using namespace storage;

namespace
{

string const kCountryId = "Angola";

string const kMapTestDir = "map-tests";

string const kTestWebServer = "http://new-search.mapswithme.com/";

void Update(LocalCountryFile const & localCountryFile)
{
  TEST_EQUAL(localCountryFile.GetCountryName(), kCountryId, ());
}

} // namespace

UNIT_TEST(StorageDownloadNodeAndDeleteNodeTests)
{
  WritableDirChanger writableDirChanger(kMapTestDir);

  Storage storage(COUNTRIES_FILE);
  TEST(version::IsSingleMwm(storage.GetCurrentDataVersion()), ());

  auto ChangeCountryFunction = [&](TCountryId const & countryId)
  {
    if (!storage.IsDownloadInProgress())
    {
      // End wait for downloading complete.
      testing::StopEventLoop();
    }
  };

  auto ProgressFunction = [&storage](TCountryId const & countryId, TLocalAndRemoteSize const & mapSize)
  {
    NodeAttrs nodeAttrs;
    storage.GetNodeAttrs(countryId, nodeAttrs);

    TEST_EQUAL(mapSize.first, nodeAttrs.m_downloadingProgress.first, (countryId));
    TEST_EQUAL(mapSize.second, nodeAttrs.m_downloadingProgress.second, (countryId));
    TEST_EQUAL(countryId, kCountryId, (countryId));
  };

  storage.Init(Update);
  storage.RegisterAllLocalMaps();
  storage.Subscribe(ChangeCountryFunction, ProgressFunction);
  storage.SetDownloadingUrlsForTesting({kTestWebServer});
  string const version = strings::to_string(storage.GetCurrentDataVersion());
  tests_support::ScopedDir cleanupVersionDir(version);

  string const mwmFullPath = my::JoinFoldersToPath({GetPlatform().WritableDir(), version},
                                                   kCountryId + DATA_FILE_EXTENSION);
  string const downloadingFullPath = my::JoinFoldersToPath(
      {GetPlatform().WritableDir(), version},
      kCountryId + DATA_FILE_EXTENSION READY_FILE_EXTENSION DOWNLOADING_FILE_EXTENSION);
  string const resumeFullPath = my::JoinFoldersToPath(
      {GetPlatform().WritableDir(), version},
      kCountryId + DATA_FILE_EXTENSION READY_FILE_EXTENSION RESUME_FILE_EXTENSION);
  Platform & platform = GetPlatform();

  // Downloading to an empty directory.
  storage.DownloadNode(kCountryId);
  // Wait for downloading complete.
  testing::RunEventLoop();

  TEST(platform.IsFileExistsByFullPath(mwmFullPath), ());
  TEST(!platform.IsFileExistsByFullPath(downloadingFullPath), ());
  TEST(!platform.IsFileExistsByFullPath(resumeFullPath), ());

  // Downloading to directory with Angola.mwm.
  storage.DownloadNode(kCountryId);

  TEST(platform.IsFileExistsByFullPath(mwmFullPath), ());
  TEST(!platform.IsFileExistsByFullPath(downloadingFullPath), ());
  TEST(!platform.IsFileExistsByFullPath(resumeFullPath), ());

  storage.DeleteNode(kCountryId);
  TEST(!platform.IsFileExistsByFullPath(mwmFullPath), ());
}
