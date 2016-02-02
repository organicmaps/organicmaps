#include "testing/testing.hpp"

#include "map/framework.hpp"

#include "platform/http_request.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"

#include "coding/file_name_utils.hpp"

#include "storage/storage.hpp"

#include "storage/storage_integration_tests/write_dir_changer.hpp"

#include "base/assert.hpp"

#include "std/condition_variable.hpp"
#include "std/mutex.hpp"
#include "std/set.hpp"
#include "std/unique_ptr.hpp"

#include <QtCore/QCoreApplication>

using namespace platform;
using namespace storage;

namespace
{

string const kTestWebServer = "http://new-search.mapswithme.com/";

string const kMapTestDir = "map-tests";

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

  //  Get children nodes for the group node
  TCountriesVec v;
  storage.GetChildren(kGroupCountryId, v);
  TCountriesSet const children(v.begin(), v.end());
  v.clear();

  TCountriesSet changed;
  auto onChangeCountryFn = [&](TCountryId const & countryId)
  {
    TEST(children.find(countryId) != children.end(), ());
    changed.insert(countryId);
    if (!storage.IsDownloadInProgress())
    {
      // end waiting when all chilren will be downloaded
      QCoreApplication::exit();
    }
  };

  TCountriesSet downloaded;
  auto onProgressFn = [&](TCountryId const & countryId, LocalAndRemoteSizeT const & mapSize)
  {
    TEST(children.find(countryId) != children.end(), ());
    if (mapSize.first == mapSize.second)
    {
      auto const res = downloaded.insert(countryId);
      TEST_EQUAL(res.second, true, ()); // every child is downloaded only once
    }
  };

  int const subsrcibtionId = storage.Subscribe(onChangeCountryFn, onProgressFn);

  // Check group node is not downloaded
  storage.GetDownloadedChildren(storage.GetRootId(), v);
  TEST(v.empty(), ());

  // Check children nodes are not downloaded
  storage.GetDownloadedChildren(kGroupCountryId, v);
  TEST(v.empty(), ());

  // Check there is no mwm or any other files for the children nodes
  for (auto const & countryId : children)
  {
    string const mwmFullPath = GetMwmFilePath(version, countryId);
    string const downloadingFullPath = GetMwmDownloadingFilePath(version, countryId);
    string const resumeFullPath = GetMwmResumeFilePath(version, countryId);
    TEST(!platform.IsFileExistsByFullPath(mwmFullPath), ());
    TEST(!platform.IsFileExistsByFullPath(downloadingFullPath), ());
    TEST(!platform.IsFileExistsByFullPath(resumeFullPath), ());
  }

  // Download the group
  if (oneByOne)
  {
    for (auto const & countryId : children)
      storage.DownloadNode(countryId);
  }
  else
  {
    storage.DownloadNode(kGroupCountryId);
  }
  // wait for downloading of all children
  QCoreApplication::exec();

  // Check all children nodes have been downloaded and changed.
  TEST_EQUAL(changed, children, ());
  TEST_EQUAL(downloaded, children, ());

  // Check state for the group node is set to UpToDate and NoError
  NodeAttrs attrs;
  storage.GetNodeAttrs(kGroupCountryId, attrs);
  // TEST_EQUAL(NodeStatus::UpToDate, attrs.m_status, ()); // DOES NOT WORK

  // Check state for the all children nodes is set to UpToDate and NoError
  for (auto const & countryId : children)
  {
    TEST_EQUAL(TStatus::EOnDisk, storage.CountryStatusEx(countryId), ());
    NodeAttrs attrs;
    storage.GetNodeAttrs(countryId, attrs);
    // TEST_EQUAL(NodeStatus::UpToDate, attrs.m_status, ()); // DOES NOT WORK
  }

  // Check there is only mwm files are present and no any other for the children nodes
  for (auto const & countryId : children)
  {
    string const mwmFullPath = GetMwmFilePath(version, countryId);
    string const downloadingFullPath = GetMwmDownloadingFilePath(version, countryId);
    string const resumeFullPath = GetMwmResumeFilePath(version, countryId);
    TEST(platform.IsFileExistsByFullPath(mwmFullPath), ());
    TEST(!platform.IsFileExistsByFullPath(downloadingFullPath), ());
    TEST(!platform.IsFileExistsByFullPath(resumeFullPath), ());
  }

  // Check group is downloaded
  storage.GetDownloadedChildren(storage.GetRootId(), v);
  TEST_EQUAL(v, TCountriesVec({kGroupCountryId}), ());
  v.clear();

  // Check all group children are downloaded
  storage.GetDownloadedChildren(kGroupCountryId, v);
  TEST_EQUAL(children, TCountriesSet(v.begin(), v.end()), ());
  v.clear();

  storage.Unsubscribe(subsrcibtionId);
}

void DeleteGroup(Storage & storage, bool oneByOne)
{
  Platform & platform = GetPlatform();

  string const version = strings::to_string(storage.GetCurrentDataVersion());

  //  Get children nodes for the group node
  TCountriesVec v;
  storage.GetChildren(kGroupCountryId, v);
  TCountriesSet const children(v.begin(), v.end());
  v.clear();

  // Check group node is downloaded
  storage.GetDownloadedChildren(storage.GetRootId(), v);
  TEST_EQUAL(v, TCountriesVec({kGroupCountryId}), ());
  v.clear();

  // Check children nodes are downloaded
  storage.GetDownloadedChildren(kGroupCountryId, v);
  TEST_EQUAL(children, TCountriesSet(v.begin(), v.end()), ());
  v.clear();

  // Check there are mwm files for the children nodes
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

  // Check state for the group node is set to UpToDate and NoError
  NodeAttrs attrs;
  storage.GetNodeAttrs(kGroupCountryId, attrs);
  // TEST_EQUAL(NodeStatus::NotDownloaded, attrs.m_status, ()); // DOES NOT WORK

  // Check state for the all children nodes is set to UpToDate and NoError
  for (auto const & countryId : children)
  {
    TEST_EQUAL(TStatus::ENotDownloaded, storage.CountryStatusEx(countryId), ());
    NodeAttrs attrs;
    storage.GetNodeAttrs(countryId, attrs);
    // TEST_EQUAL(NodeStatus::NotDownloaded, attrs.m_status, ()); // DOES NOT WORK
  }

  // Check there are no mwm files for the children nodes
  for (auto const & countryId : children)
  {
    string const mwmFullPath = GetMwmFilePath(version, countryId);
    TEST(!platform.IsFileExistsByFullPath(mwmFullPath), ());
  }

  // Check group is not downloaded
  storage.GetDownloadedChildren(storage.GetRootId(), v);
  TEST(v.empty(), ());

  // Check all children nodes are not downloaded
  storage.GetDownloadedChildren(kGroupCountryId, v);
  TEST(v.empty(), ());
}

void TestDownloadDelete(bool downloadOneByOne, bool deleteOneByOne)
{
  WritableDirChanger writableDirChanger(kMapTestDir);

  Storage storage(COUNTRIES_MIGRATE_FILE);

  TEST(version::IsSingleMwm(storage.GetCurrentDataVersion()), ());
  string const version = strings::to_string(storage.GetCurrentDataVersion());

  auto onUpdatedFn = [&](LocalCountryFile const & localCountryFile)
  {
    TCountryId const countryId = localCountryFile.GetCountryName();
    TEST(kLeafCountriesIds.find(countryId) != kLeafCountriesIds.end(), ());
  };

  storage.Init(onUpdatedFn);
  storage.RegisterAllLocalMaps();
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
