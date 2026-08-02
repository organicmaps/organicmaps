#include "testing/testing.hpp"

#include "storage/storage_integration_tests/test_defines.hpp"

#include "storage/storage.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"

#include <algorithm>
#include <string>

#include "defines.hpp"

using namespace platform;
using namespace storage;

namespace
{
int GetLevelCount(Storage & storage, CountryId const & countryId)
{
  CountriesVec children;
  storage.GetChildren(countryId, children);
  int level = 0;
  for (auto const & child : children)
    level = std::max(level, GetLevelCount(storage, child));
  return 1 + level;
}
}  // namespace

UNIT_CLASS_TEST(StorageTest, SmallMwms_3levels_Test)
{
  Platform & platform = GetPlatform();

  Storage storage;
  std::string const version = std::to_string(storage.GetCurrentDataVersion());

  CountryId country = "Germany";
  TEST_EQUAL(3, GetLevelCount(storage, country), ());

  std::string const mapDir = base::JoinPath(platform.WritableDir(), version);

  auto onProgressFn = [](CountryId const &, downloader::Progress const &) {};

  auto onChangeCountryFn = [&storage](CountryId const &)
  {
    if (!storage.IsDownloadInProgress())
      testing::StopEventLoop();
  };

  storage.Init([](CountryId const &, LocalFilePtr const) {},
               [](CountryId const &, LocalFilePtr const) { return false; });
  storage.RegisterAllLocalMaps();
  storage.Subscribe(onChangeCountryFn, onProgressFn);
  storage.SetDownloadingServersForTesting({kTestWebServer});

  /// @todo Download all Germany > 2GB takes hours here ..
  country = "Kiribati";

  NodeAttrs attrs;
  storage.GetNodeAttrs(country, attrs);
  TEST_EQUAL(attrs.m_status, NodeStatus::NotDownloaded, ());

  Platform::FilesList files;
  platform.GetFilesByExt(mapDir, DATA_FILE_EXTENSION, files);
  TEST_EQUAL(0, files.size(), ());

  storage.DownloadNode(country);
  testing::RunEventLoop();

  storage.GetNodeAttrs(country, attrs);
  TEST_EQUAL(attrs.m_status, NodeStatus::OnDisk, ());

  files.clear();
  platform.GetFilesByExt(mapDir, DATA_FILE_EXTENSION, files);
  TEST_GREATER(files.size(), 0, ());

  storage.DeleteNode(country);

  storage.GetNodeAttrs(country, attrs);
  TEST_EQUAL(attrs.m_status, NodeStatus::NotDownloaded, ());

  files.clear();
  platform.GetFilesByExt(mapDir, DATA_FILE_EXTENSION, files);
  TEST_EQUAL(0, files.size(), ());
}
