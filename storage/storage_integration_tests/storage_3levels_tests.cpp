#include "testing/testing.hpp"

#include "storage/storage_integration_tests/test_defines.hpp"

#include "map/framework.hpp"

#include "platform/platform.hpp"
#include "platform/platform_tests_support/writable_dir_changer.hpp"

#include "base/file_name_utils.hpp"

#include <algorithm>
#include <string>

#include "defines.hpp"

using namespace platform;
using namespace storage;

namespace
{
std::string const kCountryId = "Germany";  // Germany has 3-levels hierachy

int GetLevelCount(Storage & storage, CountryId const & countryId)
{
  CountriesVec children;
  storage.GetChildren(countryId, children);
  int level = 0;
  for (auto const & child : children)
    level = std::max(level, GetLevelCount(storage, child));
  return 1 + level;
}
} // namespace

UNIT_TEST(SmallMwms_3levels_Test)
{
  WritableDirChanger writableDirChanger(kMapTestDir);

  Platform & platform = GetPlatform();

  Framework f(FrameworkParams(false /* m_enableLocalAds */, false /* m_enableDiffs */));
  auto & storage = f.GetStorage();
  std::string const version = strings::to_string(storage.GetCurrentDataVersion());

  TEST_EQUAL(3, GetLevelCount(storage, kCountryId), ());

  std::string const mapDir = base::JoinPath(platform.WritableDir(), version);

  auto onProgressFn = [&](CountryId const & countryId, downloader::Progress const & /* progress */) {};

  auto onChangeCountryFn = [&](CountryId const & countryId) {
    if (!storage.IsDownloadInProgress())
      testing::StopEventLoop();
  };

  storage.Subscribe(onChangeCountryFn, onProgressFn);
  storage.SetDownloadingServersForTesting({kTestWebServer});

  NodeAttrs attrs;
  storage.GetNodeAttrs(kCountryId, attrs);
  TEST_EQUAL(attrs.m_status, NodeStatus::NotDownloaded, ());

  Platform::FilesList files;
  platform.GetFilesByExt(mapDir, DATA_FILE_EXTENSION, files);
  TEST_EQUAL(0, files.size(), ());

  storage.DownloadNode(kCountryId);
  testing::RunEventLoop();

  storage.GetNodeAttrs(kCountryId, attrs);
  TEST_EQUAL(attrs.m_status, NodeStatus::OnDisk, ());

  files.clear();
  platform.GetFilesByExt(mapDir, DATA_FILE_EXTENSION, files);
  TEST_GREATER(files.size(), 0, ());

  storage.DeleteNode(kCountryId);

  storage.GetNodeAttrs(kCountryId, attrs);
  TEST_EQUAL(attrs.m_status, NodeStatus::NotDownloaded, ());

  files.clear();
  platform.GetFilesByExt(mapDir, DATA_FILE_EXTENSION, files);
  TEST_EQUAL(0, files.size(), ());
}
