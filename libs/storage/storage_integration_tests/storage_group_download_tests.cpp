#include "testing/testing.hpp"

#include "storage/storage_integration_tests/test_defines.hpp"

#include "map/framework.hpp"

#include "platform/http_request.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/writable_dir_changer.hpp"

#include "storage/storage.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"

#include <string>

namespace storage_group_download_tests
{
using namespace platform;
using namespace std;
using namespace storage;

CountryId const kGroupCountryId = "Venezuela";
CountriesSet const kLeafCountriesIds = {"Venezuela_North", "Venezuela_South"};

string GetMwmFilePath(string const & version, CountryId const & countryId)
{
  return base::JoinPath(GetPlatform().WritableDir(), version, countryId + DATA_FILE_EXTENSION);
}

string GetMwmDownloadingFilePath(string const & version, CountryId const & countryId)
{
  return base::JoinPath(GetPlatform().WritableDir(), version,
                        countryId + DATA_FILE_EXTENSION READY_FILE_EXTENSION DOWNLOADING_FILE_EXTENSION);
}

string GetMwmResumeFilePath(string const & version, CountryId const & countryId)
{
  return base::JoinPath(GetPlatform().WritableDir(), version,
                        countryId + DATA_FILE_EXTENSION READY_FILE_EXTENSION RESUME_FILE_EXTENSION);
}

void DownloadGroup(Storage & storage, bool oneByOne)
{
  Platform & platform = GetPlatform();

  string const version = strings::to_string(storage.GetCurrentDataVersion());

  //  Get children nodes for the group node.
  CountriesVec children;
  //  All nodes in subtree (including the root) for the group node.
  storage.GetChildren(kGroupCountryId, children);
  CountriesSet subTree;
  storage.ForEachInSubtree(kGroupCountryId, [&subTree](CountryId const & descendantId, bool /* groupNode */)
  { subTree.insert(descendantId); });

  CountriesSet changed;
  auto onChangeCountryFn = [&](CountryId const & countryId)
  {
    TEST(subTree.find(countryId) != subTree.end(), (countryId));
    changed.insert(countryId);
    if (!storage.IsDownloadInProgress())
    {
      // Stop waiting when all chilren will be downloaded.
      testing::StopEventLoop();
    }
  };

  CountriesSet downloadedChecker;
  auto onProgressFn = [&](CountryId const & countryId, downloader::Progress const & progress)
  {
    TEST(subTree.find(countryId) != subTree.end(), ());
    if (progress.m_bytesDownloaded == progress.m_bytesTotal)
    {
      auto const res = downloadedChecker.insert(countryId);
      TEST_EQUAL(res.second, true, ());  // Every child is downloaded only once.
    }
  };

  int const subsrcibtionId = storage.Subscribe(onChangeCountryFn, onProgressFn);

  // Check group node is not downloaded
  CountriesVec downloaded, available;
  storage.GetChildrenInGroups(storage.GetRootId(), downloaded, available);
  TEST(downloaded.empty(), ());

  // Check children nodes are not downloaded
  storage.GetChildrenInGroups(kGroupCountryId, downloaded, available);
  TEST(downloaded.empty(), ());

  // Check status for the all children nodes is set to NotDownloaded.
  MwmSize totalGroupSize = 0;
  for (auto const & countryId : children)
  {
    TEST_EQUAL(Status::NotDownloaded, storage.CountryStatusEx(countryId), ());
    NodeAttrs attrs;
    storage.GetNodeAttrs(countryId, attrs);
    TEST_EQUAL(NodeStatus::NotDownloaded, attrs.m_status, ());
    TEST_GREATER(attrs.m_mwmSize, 0, ());
    totalGroupSize += attrs.m_mwmSize;
  }

  // Check status for the group node is set to NotDownloaded.
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
    for (auto const & countryId : children)
      storage.DownloadNode(countryId);
  else
    storage.DownloadNode(kGroupCountryId);
  // Wait for downloading of all children.
  testing::RunEventLoop();

  // Check if all nodes in the subtree have been downloaded and changed.
  TEST_EQUAL(changed, subTree, ());
  TEST_EQUAL(downloadedChecker, subTree, ());

  // Check status for the group node is set to OnDisk.
  storage.GetNodeAttrs(kGroupCountryId, attrs);
  TEST_EQUAL(NodeStatus::OnDisk, attrs.m_status, ());

  // Check status for the all children nodes is set to OnDisk.
  for (auto const & countryId : children)
  {
    TEST_EQUAL(Status::OnDisk, storage.CountryStatusEx(countryId), ());
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
  TEST_EQUAL(downloaded, CountriesVec({kGroupCountryId}), ());

  // Check all group children are downloaded.
  storage.GetChildrenInGroups(kGroupCountryId, downloaded, available);
  TEST_EQUAL(CountriesSet(children.begin(), children.end()), CountriesSet(downloaded.begin(), downloaded.end()), ());

  storage.Unsubscribe(subsrcibtionId);
}

void DeleteGroup(Storage & storage, bool oneByOne)
{
  Platform & platform = GetPlatform();

  string const version = strings::to_string(storage.GetCurrentDataVersion());

  //  Get children nodes for the group node.
  CountriesVec v;
  storage.GetChildren(kGroupCountryId, v);
  CountriesSet const children(v.begin(), v.end());
  v.clear();

  // Check group node is downloaded.
  CountriesVec downloaded, available;
  storage.GetChildrenInGroups(storage.GetRootId(), downloaded, available);
  TEST_EQUAL(downloaded, CountriesVec({kGroupCountryId}), ());

  // Check children nodes are downloaded.
  storage.GetChildrenInGroups(kGroupCountryId, downloaded, available);
  TEST_EQUAL(children, CountriesSet(downloaded.begin(), downloaded.end()), ());

  // Check there are mwm files for the children nodes.
  for (auto const & countryId : children)
  {
    string const mwmFullPath = GetMwmFilePath(version, countryId);
    TEST(platform.IsFileExistsByFullPath(mwmFullPath), ());
  }

  // Delete the group
  if (oneByOne)
    for (auto const & countryId : children)
      storage.DeleteNode(countryId);
  else
    storage.DeleteNode(kGroupCountryId);

  // Check state for the group node is set to NotDownloaded and NoError.
  NodeAttrs attrs;
  storage.GetNodeAttrs(kGroupCountryId, attrs);
  TEST_EQUAL(NodeStatus::NotDownloaded, attrs.m_status, ());

  // Check state for the all children nodes is set to NotDownloaded and NoError.
  for (auto const & countryId : children)
  {
    TEST_EQUAL(Status::NotDownloaded, storage.CountryStatusEx(countryId), ());
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

  Storage storage;
  string const version = strings::to_string(storage.GetCurrentDataVersion());

  auto onUpdatedFn = [&](CountryId const &, storage::LocalFilePtr const localCountryFile)
  {
    CountryId const countryId = localCountryFile->GetCountryName();
    TEST(kLeafCountriesIds.find(countryId) != kLeafCountriesIds.end(), ());
  };

  storage.Init(onUpdatedFn, [](CountryId const &, storage::LocalFilePtr const) { return false; });
  storage.RegisterAllLocalMaps();
  storage.SetDownloadingServersForTesting({kTestWebServer});

  tests_support::ScopedDir cleanupVersionDir(version);

  // Check children for the kGroupCountryId
  CountriesVec children;
  storage.GetChildren(kGroupCountryId, children);
  TEST_EQUAL(CountriesSet(children.begin(), children.end()), kLeafCountriesIds, ());

  DownloadGroup(storage, downloadOneByOne);

  DeleteGroup(storage, deleteOneByOne);
}

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
}  // namespace storage_group_download_tests
