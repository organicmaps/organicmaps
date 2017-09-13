#include "testing/testing.hpp"

#include "storage/storage_integration_tests/test_defines.hpp"

#include "storage/storage.hpp"

#include "platform/local_country_file_utils.hpp"
#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/writable_dir_changer.hpp"

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

void Update(TCountryId const &, TLocalFilePtr const localCountryFile)
{
  TEST_EQUAL(localCountryFile->GetCountryName(), kCountryId, ());
}

void UpdateWithoutChecks(TCountryId const &, TLocalFilePtr const /* localCountryFile */)
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

  storage.Init(didDownload, [](TCountryId const &, TLocalFilePtr const){return false;});
  storage.RegisterAllLocalMaps(false /* enableDiffs */);
  storage.Subscribe(changeCountryFunction, progress);
  storage.SetDownloadingUrlsForTesting({kTestWebServer});
}

class StorageHttpTest
{
protected:
  WritableDirChanger const m_writableDirChanger;
  Storage m_storage;
  string const m_version;
  tests_support::ScopedDir const m_cleanupVersionDir;

public:
  StorageHttpTest()
    : m_writableDirChanger(kMapTestDir), m_storage(COUNTRIES_FILE),
      m_version(strings::to_string(m_storage.GetCurrentDataVersion())),
      m_cleanupVersionDir(m_version)
  {
    TEST(version::IsSingleMwm(m_storage.GetCurrentDataVersion()), ());
  }
};
}  // namespace

UNIT_CLASS_TEST(StorageHttpTest, StorageDownloadNodeAndDeleteNode)
{
  auto const progressFunction = [this](TCountryId const & countryId, TLocalAndRemoteSize const & mapSize)
  {
    NodeAttrs nodeAttrs;
    m_storage.GetNodeAttrs(countryId, nodeAttrs);

    TEST_EQUAL(mapSize.first, nodeAttrs.m_downloadingProgress.first, (countryId));
    TEST_EQUAL(mapSize.second, nodeAttrs.m_downloadingProgress.second, (countryId));
    TEST_EQUAL(countryId, kCountryId, (countryId));
  };

  InitStorage(m_storage, Update, progressFunction);

  string const mwmFullPath = GetMwmFullPath(kCountryId, m_version);
  string const downloadingFullPath = GetDownloadingFullPath(kCountryId, m_version);
  string const resumeFullPath = GetResumeFullPath(kCountryId, m_version);

  // Downloading to an empty directory.
  m_storage.DownloadNode(kCountryId);
  // Wait for downloading complete.
  testing::RunEventLoop();

  Platform & platform = GetPlatform();
  TEST(platform.IsFileExistsByFullPath(mwmFullPath), ());
  TEST(!platform.IsFileExistsByFullPath(downloadingFullPath), ());
  TEST(!platform.IsFileExistsByFullPath(resumeFullPath), ());

  // Downloading to directory with Angola.mwm.
  m_storage.DownloadNode(kCountryId);

  TEST(platform.IsFileExistsByFullPath(mwmFullPath), ());
  TEST(!platform.IsFileExistsByFullPath(downloadingFullPath), ());
  TEST(!platform.IsFileExistsByFullPath(resumeFullPath), ());

  m_storage.DeleteNode(kCountryId);
  TEST(!platform.IsFileExistsByFullPath(mwmFullPath), ());
}

UNIT_CLASS_TEST(StorageHttpTest, StorageDownloadAndDeleteDisputedNode)
{
  auto const progressFunction = [this](TCountryId const & countryId,
      TLocalAndRemoteSize const & mapSize)
  {
    NodeAttrs nodeAttrs;
    m_storage.GetNodeAttrs(countryId, nodeAttrs);

    TEST_EQUAL(mapSize.first, nodeAttrs.m_downloadingProgress.first, (countryId));
    TEST_EQUAL(mapSize.second, nodeAttrs.m_downloadingProgress.second, (countryId));
  };

  InitStorage(m_storage, UpdateWithoutChecks, progressFunction);

  string const mwmFullPath1 = GetMwmFullPath(kDisputedCountryId1, m_version);
  string const mwmFullPath2 = GetMwmFullPath(kDisputedCountryId2, m_version);
  string const mwmFullPath3 = GetMwmFullPath(kDisputedCountryId3, m_version);
  string const mwmFullPathUndisputed = GetMwmFullPath(kUndisputedCountryId, m_version);

  // Downloading to an empty directory.
  m_storage.DownloadNode(kDisputedCountryId1);
  m_storage.DownloadNode(kDisputedCountryId2);
  m_storage.DownloadNode(kDisputedCountryId3);
  m_storage.DownloadNode(kUndisputedCountryId);
  // Wait for downloading complete.
  testing::RunEventLoop();

  Platform & platform = GetPlatform();
  TEST(platform.IsFileExistsByFullPath(mwmFullPath1), ());
  TEST(platform.IsFileExistsByFullPath(mwmFullPath2), ());
  TEST(platform.IsFileExistsByFullPath(mwmFullPath3), ());
  TEST(platform.IsFileExistsByFullPath(mwmFullPathUndisputed), ());

  TCountriesVec downloadedChildren;
  TCountriesVec availChildren;
  m_storage.GetChildrenInGroups(m_storage.GetRootId(), downloadedChildren, availChildren);

  TCountriesVec const expectedDownloadedChildren = {"Argentina", kDisputedCountryId2,  kDisputedCountryId1};
  TEST_EQUAL(downloadedChildren, expectedDownloadedChildren, ());
  TEST_EQUAL(availChildren.size(), 223, ());

  m_storage.DeleteNode(kDisputedCountryId1);
  m_storage.DeleteNode(kDisputedCountryId2);
  m_storage.DeleteNode(kDisputedCountryId3);
  m_storage.DeleteNode(kUndisputedCountryId);
  TEST(!platform.IsFileExistsByFullPath(mwmFullPath1), ());
  TEST(!platform.IsFileExistsByFullPath(mwmFullPath2), ());
  TEST(!platform.IsFileExistsByFullPath(mwmFullPath3), ());
  TEST(!platform.IsFileExistsByFullPath(mwmFullPathUndisputed), ());
}
