#include "testing/testing.hpp"

#include "storage/storage_integration_tests/test_defines.hpp"

#include "map/framework.hpp"

#include "platform/http_request.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/writable_dir_changer.hpp"

#include "coding/file_name_utils.hpp"

#include "storage/storage.hpp"

#include "base/assert.hpp"

#include "std/condition_variable.hpp"
#include "std/mutex.hpp"
#include "std/set.hpp"
#include "std/unique_ptr.hpp"

using namespace platform;
using namespace storage;

namespace
{
TCountryId const kGroupCountryId = "New Zealand";
TCountriesSet const kLeafCountriesIds = {"Tokelau",
                                         "New Zealand North_Auckland",
                                         "New Zealand North_Wellington",
                                         "New Zealand South_Canterbury",
                                         "New Zealand South_Southland"};

string GetMwmFilePath(string const & version, TCountryId const & countryId)
{
  return my::JoinFoldersToPath({GetPlatform().WritableDir(), version},
                               countryId + DATA_FILE_EXTENSION);
}

string GetMwmDownloadingFilePath(string const & version, TCountryId const & countryId)
{
  return my::JoinFoldersToPath({GetPlatform().WritableDir(), version},
                               countryId + DATA_FILE_EXTENSION READY_FILE_EXTENSION DOWNLOADING_FILE_EXTENSION);
}

string GetMwmResumeFilePath(string const & version, TCountryId const & countryId)
{
  return my::JoinFoldersToPath({GetPlatform().WritableDir(), version},
                               countryId + DATA_FILE_EXTENSION READY_FILE_EXTENSION RESUME_FILE_EXTENSION);
}

void DownloadGroup(Storage & storage, bool oneByOne)
{
  Platform & platform = GetPlatform();

  string const version = strings::to_string(storage.GetCurrentDataVersion());

  //  Get children nodes for the group node.
  TCountriesVec children;
  //  All nodes in subtree (including the root) for the group node.
  storage.GetChildren(kGroupCountryId, children);
  TCountriesSet subTree;
  storage.ForEachInSubtree(kGroupCountryId, [&subTree](TCountryId const & descendantId, bool /* groupNode */)
  {
    subTree.insert(descendantId);
  });

  TCountriesSet changed;
  auto onChangeCountryFn = [&](TCountryId const & countryId)
  {
    TEST(subTree.find(countryId) != subTree.end(), (countryId));
    changed.insert(countryId);
    if (!storage.IsDownloadInProgress())
    {
      // Stop waiting when all chilren will be downloaded.
      testing::StopEventLoop();
    }
  };

  TCountriesSet downloadedChecker;
  auto onProgressFn = [&](TCountryId const & countryId, TLocalAndRemoteSize const & mapSize)
  {
    TEST(subTree.find(countryId) != subTree.end(), ());
    if (mapSize.first == mapSize.second)
    {
      auto const res = downloadedChecker.insert(countryId);
      TEST_EQUAL(res.second, true, ()); // Every child is downloaded only once.
    }
  };

  int const subsrcibtionId = storage.Subscribe(onChangeCountryFn, onProgressFn);

  // Check group node is not downloaded
  TCountriesVec downloaded, available;
  storage.GetChildrenInGroups(storage.GetRootId(), downloaded, available);
  TEST(downloaded.empty(), ());

  // Check children nodes are not downloaded
  storage.GetChildrenInGroups(kGroupCountryId, downloaded, available);
  TEST(downloaded.empty(), ());

  // Check status for the all children nodes is set to ENotDownloaded.
  TMwmSize totalGroupSize = 0;
  for (auto const & countryId : children)
  {
    TEST_EQUAL(Status::ENotDownloaded, storage.CountryStatusEx(countryId), ());
    NodeAttrs attrs;
    storage.GetNodeAttrs(countryId, attrs);
    TEST_EQUAL(NodeStatus::NotDownloaded, attrs.m_status, ());
    TEST_GREATER(attrs.m_mwmSize, 0, ());
    totalGroupSize += attrs.m_mwmSize;
  }

  // Check status for the group node is set to ENotDownloaded.
  NodeAttrs attrs;
  storage.GetNodeAttrs(kGroupCountryId, attrs);
  TEST_EQUAL(NodeStatus::NotDownloaded, attrs.m_status, ());
  TEST_EQUAL(attrs.m_mwmSize, totalGroupSize, ());
  attrs = NodeAttrs();

  // Check there is no mwm or any other files for the children nodes.
  for (auto const & countryId : children)
  {
    string const mwmFullPath = GetMwmFilePath(version, countryId);
    string const downloadingFullPath = GetMwmDownloadingFilePath(version, countryId);
    string const resumeFullPath = GetMwmResumeFilePath(version, countryId);
    TEST(!platform.IsFileExistsByFullPath(mwmFullPath), ());
    TEST(!platform.IsFileExistsByFullPath(downloadingFullPath), ());
    TEST(!platform.IsFileExistsByFullPath(resumeFullPath), ());
  }

  // Download the group.
  if (oneByOne)
  {
    for (auto const & countryId : children)
      storage.DownloadNode(countryId);
  }
  else
  {
    storage.DownloadNode(kGroupCountryId);
  }
  // Wait for downloading of all children.
  testing::RunEventLoop();

  // Check if all nodes in the subtree have been downloaded and changed.
  TEST_EQUAL(changed, subTree, ());
  TEST_EQUAL(downloadedChecker, subTree, ());

  // Check status for the group node is set to EOnDisk.
  storage.GetNodeAttrs(kGroupCountryId, attrs);
  TEST_EQUAL(NodeStatus::OnDisk, attrs.m_status, ());

  // Check status for the all children nodes is set to EOnDisk.
  for (auto const & countryId : children)
  {
    TEST_EQUAL(Status::EOnDisk, storage.CountryStatusEx(countryId), ());
    NodeAttrs attrs;
    storage.GetNodeAttrs(countryId, attrs);
    TEST_EQUAL(NodeStatus::OnDisk, attrs.m_status, ());
  }

  // Check there is only mwm files are present and no any other for the children nodes.
  for (auto const & countryId : children)
  {
    string const mwmFullPath = GetMwmFilePath(version, countryId);
    string const downloadingFullPath = GetMwmDownloadingFilePath(version, countryId);
    string const resumeFullPath = GetMwmResumeFilePath(version, countryId);
    TEST(platform.IsFileExistsByFullPath(mwmFullPath), ());
    TEST(!platform.IsFileExistsByFullPath(downloadingFullPath), ());
    TEST(!platform.IsFileExistsByFullPath(resumeFullPath), ());
  }

  // Check group is downloaded.
  storage.GetChildrenInGroups(storage.GetRootId(), downloaded, available);
  TEST_EQUAL(downloaded, TCountriesVec({kGroupCountryId}), ());

  // Check all group children are downloaded.
  storage.GetChildrenInGroups(kGroupCountryId, downloaded, available);
  TEST_EQUAL(TCountriesSet(children.begin(), children.end()),
             TCountriesSet(downloaded.begin(), downloaded.end()), ());

  storage.Unsubscribe(subsrcibtionId);
}

void DeleteGroup(Storage & storage, bool oneByOne)
{
  Platform & platform = GetPlatform();

  string const version = strings::to_string(storage.GetCurrentDataVersion());

  //  Get children nodes for the group node.
  TCountriesVec v;
  storage.GetChildren(kGroupCountryId, v);
  TCountriesSet const children(v.begin(), v.end());
  v.clear();

  // Check group node is downloaded.
  TCountriesVec downloaded, available;
  storage.GetChildrenInGroups(storage.GetRootId(), downloaded, available);
  TEST_EQUAL(downloaded, TCountriesVec({kGroupCountryId}), ());

  // Check children nodes are downloaded.
  storage.GetChildrenInGroups(kGroupCountryId, downloaded, available);
  TEST_EQUAL(children, TCountriesSet(downloaded.begin(), downloaded.end()), ());

  // Check there are mwm files for the children nodes.
  for (auto const & countryId : children)
  {
    string const mwmFullPath = GetMwmFilePath(version, countryId);
    TEST(platform.IsFileExistsByFullPath(mwmFullPath), ());
  }

  // Delete the group
  if (oneByOne)
  {
    for (auto const & countryId : children)
      storage.DeleteNode(countryId);
  }
  else
  {
    storage.DeleteNode(kGroupCountryId);
  }

  // Check state for the group node is set to NotDownloaded and NoError.
  NodeAttrs attrs;
  storage.GetNodeAttrs(kGroupCountryId, attrs);
  TEST_EQUAL(NodeStatus::NotDownloaded, attrs.m_status, ());

  // Check state for the all children nodes is set to NotDownloaded and NoError.
  for (auto const & countryId : children)
  {
    TEST_EQUAL(Status::ENotDownloaded, storage.CountryStatusEx(countryId), ());
    NodeAttrs attrs;
    storage.GetNodeAttrs(countryId, attrs);
    TEST_EQUAL(NodeStatus::NotDownloaded, attrs.m_status, ());
  }

  // Check there are no mwm files for the children nodes.
  for (auto const & countryId : children)
  {
    string const mwmFullPath = GetMwmFilePath(version, countryId);
    TEST(!platform.IsFileExistsByFullPath(mwmFullPath), ());
  }

  // Check group is not downloaded.
  storage.GetChildrenInGroups(storage.GetRootId(), downloaded, available);
  TEST(downloaded.empty(), ());

  // Check all children nodes are not downloaded.
  storage.GetChildrenInGroups(kGroupCountryId, downloaded, available);
  TEST(downloaded.empty(), ());
}

void TestDownloadDelete(bool downloadOneByOne, bool deleteOneByOne)
{
  WritableDirChanger writableDirChanger(kMapTestDir);

  Storage storage(COUNTRIES_FILE);

  TEST(version::IsSingleMwm(storage.GetCurrentDataVersion()), ());
  string const version = strings::to_string(storage.GetCurrentDataVersion());

  auto onUpdatedFn = [&](TCountryId const &, storage::TLocalFilePtr const localCountryFile)
  {
    TCountryId const countryId = localCountryFile->GetCountryName();
    TEST(kLeafCountriesIds.find(countryId) != kLeafCountriesIds.end(), ());
  };

  storage.Init(onUpdatedFn, [](TCountryId const &, storage::TLocalFilePtr const){return false;});
  storage.RegisterAllLocalMaps(false /* enableDiffs */);
  storage.SetDownloadingUrlsForTesting({kTestWebServer});

  tests_support::ScopedDir cleanupVersionDir(version);

  // Check children for the kGroupCountryId
  TCountriesVec children;
  storage.GetChildren(kGroupCountryId, children);
  TEST_EQUAL(TCountriesSet(children.begin(), children.end()), kLeafCountriesIds, ());

  DownloadGroup(storage, downloadOneByOne);

  DeleteGroup(storage, deleteOneByOne);
}

} // namespace

UNIT_TEST(SmallMwms_GroupDownloadDelete_Test1)
{
  TestDownloadDelete(false, false);
}

UNIT_TEST(SmallMwms_GroupDownloadDelete_Test2)
{
  TestDownloadDelete(false, true);
}

UNIT_TEST(SmallMwms_GroupDownloadDelete_Test3)
{
  TestDownloadDelete(true, false);
}

UNIT_TEST(SmallMwms_GroupDownloadDelete_Test4)
{
  TestDownloadDelete(true, true);
}
