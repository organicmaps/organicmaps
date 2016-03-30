#include "testing/testing.hpp"

#include "storage/storage_integration_tests/test_defines.hpp"

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
string const kDisputedCountryId1 = "Jerusalem";
string const kDisputedCountryId2 = "Crimea";
string const kDisputedCountryId3 = "Campo de Hielo Sur";
string const kUndisputedCountryId = "Argentina_Buenos Aires_North";

void Update(TCountryId const &, Storage::TLocalFilePtr const localCountryFile)
{
  TEST_EQUAL(localCountryFile->GetCountryName(), kCountryId, ());
}

void UpdateWithoutChecks(TCountryId const &, Storage::TLocalFilePtr const /* localCountryFile */)
{
}

string const GetMwmFullPath(string const & countryId, string const & version)
{
  return my::JoinFoldersToPath({GetPlatform().WritableDir(), version},
                               countryId + DATA_FILE_EXTENSION);
}

string const GetDownloadingFullPath(string const & countryId, string const & version)
{
  return my::JoinFoldersToPath({GetPlatform().WritableDir(), version},
                               kCountryId + DATA_FILE_EXTENSION READY_FILE_EXTENSION DOWNLOADING_FILE_EXTENSION);
}

string const GetResumeFullPath(string const & countryId, string const & version)
{
  return my::JoinFoldersToPath({GetPlatform().WritableDir(), version},
                               kCountryId + DATA_FILE_EXTENSION READY_FILE_EXTENSION RESUME_FILE_EXTENSION);
}

void InitStorage(Storage & storage, Storage::TUpdateCallback const & didDownload,
                 Storage::TProgressFunction const & progress)
{
  TEST(version::IsSingleMwm(storage.GetCurrentDataVersion()), ());

  auto const changeCountryFunction = [&](TCountryId const & /* countryId */)
  {
    if (!storage.IsDownloadInProgress())
    {
      // End wait for downloading complete.
      testing::StopEventLoop();
    }
  };

  storage.Init(didDownload, [](TCountryId const &, Storage::TLocalFilePtr const){return false;});
  storage.RegisterAllLocalMaps();
  storage.Subscribe(changeCountryFunction, progress);
  storage.SetDownloadingUrlsForTesting({kTestWebServer});
}
} // namespace

UNIT_TEST(StorageDownloadNodeAndDeleteNodeTests)
{
  WritableDirChanger writableDirChanger(kMapTestDir);

  Storage storage(COUNTRIES_FILE);

  auto const progressFunction = [&storage](TCountryId const & countryId, TLocalAndRemoteSize const & mapSize)
  {
    NodeAttrs nodeAttrs;
    storage.GetNodeAttrs(countryId, nodeAttrs);

    TEST_EQUAL(mapSize.first, nodeAttrs.m_downloadingProgress.first, (countryId));
    TEST_EQUAL(mapSize.second, nodeAttrs.m_downloadingProgress.second, (countryId));
    TEST_EQUAL(countryId, kCountryId, (countryId));
  };

  InitStorage(storage, Update, progressFunction);

  string const version = strings::to_string(storage.GetCurrentDataVersion());
  tests_support::ScopedDir cleanupVersionDir(version);

  string const mwmFullPath = GetMwmFullPath(kCountryId, version);
  string const downloadingFullPath = GetDownloadingFullPath(kCountryId, version);
  string const resumeFullPath = GetResumeFullPath(kCountryId, version);

  // Downloading to an empty directory.
  storage.DownloadNode(kCountryId);
  // Wait for downloading complete.
  testing::RunEventLoop();

  Platform & platform = GetPlatform();
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

UNIT_TEST(StorageDownloadAndDeleteDisputedNodeTests)
{
  WritableDirChanger writableDirChanger(kMapTestDir);

  Storage storage(COUNTRIES_FILE);
  auto const progressFunction = [&storage](TCountryId const & countryId,
      TLocalAndRemoteSize const & mapSize)
  {
    NodeAttrs nodeAttrs;
    storage.GetNodeAttrs(countryId, nodeAttrs);

    TEST_EQUAL(mapSize.first, nodeAttrs.m_downloadingProgress.first, (countryId));
    TEST_EQUAL(mapSize.second, nodeAttrs.m_downloadingProgress.second, (countryId));
  };

  InitStorage(storage, UpdateWithoutChecks, progressFunction);

  string const version = strings::to_string(storage.GetCurrentDataVersion());
  tests_support::ScopedDir cleanupVersionDir(version);

  string const mwmFullPath1 = GetMwmFullPath(kDisputedCountryId1, version);
  string const mwmFullPath2 = GetMwmFullPath(kDisputedCountryId2, version);
  string const mwmFullPath3 = GetMwmFullPath(kDisputedCountryId3, version);
  string const mwmFullPathUndisputed = GetMwmFullPath(kUndisputedCountryId, version);

  // Downloading to an empty directory.
  storage.DownloadNode(kDisputedCountryId1);
  storage.DownloadNode(kDisputedCountryId2);
  storage.DownloadNode(kDisputedCountryId3);
  storage.DownloadNode(kUndisputedCountryId);
  // Wait for downloading complete.
  testing::RunEventLoop();

  Platform & platform = GetPlatform();
  TEST(platform.IsFileExistsByFullPath(mwmFullPath1), ());
  TEST(platform.IsFileExistsByFullPath(mwmFullPath2), ());
  TEST(platform.IsFileExistsByFullPath(mwmFullPath3), ());
  TEST(platform.IsFileExistsByFullPath(mwmFullPathUndisputed), ());

  TCountriesVec downloadedChildren;
  TCountriesVec availChildren;
  storage.GetChildrenInGroups(storage.GetRootId(), downloadedChildren, availChildren);

  TCountriesVec const expectedDownloadedChildren = {"Argentina", kDisputedCountryId2,  kDisputedCountryId1};
  TEST_EQUAL(downloadedChildren, expectedDownloadedChildren, ());
  TEST_EQUAL(availChildren.size(), 221, ());

  storage.DeleteNode(kDisputedCountryId1);
  storage.DeleteNode(kDisputedCountryId2);
  storage.DeleteNode(kDisputedCountryId3);
  storage.DeleteNode(kUndisputedCountryId);
  TEST(!platform.IsFileExistsByFullPath(mwmFullPath1), ());
  TEST(!platform.IsFileExistsByFullPath(mwmFullPath2), ());
  TEST(!platform.IsFileExistsByFullPath(mwmFullPath3), ());
  TEST(!platform.IsFileExistsByFullPath(mwmFullPathUndisputed), ());
}
