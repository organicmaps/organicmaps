#include "testing/testing.hpp"

#include "storage/storage_tests/fake_map_files_downloader.hpp"
#include "storage/storage_tests/task_runner.hpp"

#include "storage/downloading_policy.hpp"
#include "storage/storage.hpp"

#include "indexer/terrain/terrain_serdes.hpp"

#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"
#include "platform/platform_tests_support/writable_dir_changer.hpp"
#include "platform/settings.hpp"

#include "geometry/mercator.hpp"

#include "coding/blake3.hpp"
#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/files_container.hpp"
#include "coding/point_coding.hpp"

#include "base/file_name_utils.hpp"
#include "base/string_utils.hpp"

#include "defines.hpp"

#include <algorithm>
#include <array>
#include <future>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace storage_terrain_tests
{
namespace tests_support = platform::tests_support;
using namespace storage;
using terrain::TerrainId;

static double constexpr kRectCompareEpsilon = 1e-2;

// The terrain tests constructing a Storage must be isolated twice over:
// - WritableDirChanger keeps the terrain files and (in a filtered run) the settings in
//   a temp dir - the artifact sweep deletes the downloader leftovers and must not
//   touch a real data/terrain;
// - the settings singleton binds its file at the FIRST use in the process, so in a full
//   suite run an earlier unguarded test pins it to the real settings; the keys the
//   terrain paths write (or act upon: a real DownloadQueue value would make
//   RestoreDownloadQueue enqueue real downloads) are saved/cleared/restored explicitly.
char const kTerrainTestDir[] = "terrain_tests";

// A fake current-version local map must MATCH its countries.json size (an
// inconsistent pair logs an error, which aborts the tests) - a sparse resize costs
// no disk and no time.
void ResizeToRemote(tests_support::ScopedFile const & file, Storage const & storage, CountryId const & id)
{
  FileWriter writer(file.GetFullPath());
  writer.Seek(storage.GetCountryFile(id).GetRemoteSize() - 1);
  writer.Write("", 1);
}

class ScopedDownloadQueue
{
public:
  ScopedDownloadQueue()
  {
    for (auto const * key : {"DownloadQueue", "TerrainDownloadQueue", "TerrainRequestedRegions"})
    {
      std::string value;
      if (settings::Get(key, value))
        m_saved.emplace(key, std::move(value));
      settings::Delete(key);
    }
  }

  ~ScopedDownloadQueue()
  {
    for (auto const * key : {"DownloadQueue", "TerrainDownloadQueue", "TerrainRequestedRegions"})
      if (auto const it = m_saved.find(key); it != m_saved.end())
        settings::Set(key, it->second);
      else
        settings::Delete(key);
  }

private:
  std::map<std::string, std::string> m_saved;
};

terrain::TwmFile MakeTerrainFile(Storage::TerrainBlock const & block, int64_t version)
{
  return {block.m_id, version,
          base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR, strings::to_string(version),
                         block.m_id + TERRAIN_FILE_EXT),
          block.m_rect};
}

// Test files contain stub bytes. Supply the descriptors that the real header scan
// would produce, using catalog rectangles for these current-grid blocks.
std::vector<terrain::TwmFile> ScanTerrain(Storage & storage)
{
  int64_t gridVersion = 0;
  std::vector<Storage::TerrainBlock> blocks;
  std::map<CountryId, std::vector<uint32_t>> coverage;
  std::string content;
  GetPlatform().GetReader(TERRAIN_GRID_FILE)->ReadAsString(content);
  Storage::ParseTwmGridJson(content, gridVersion, blocks, coverage);

  std::vector<terrain::TwmFile> scanned;
  for (auto const & [dir, version] : terrain::ListVersionDirs(base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR)))
  {
    Platform::FilesList files;
    Platform::GetFilesByExt(dir, TERRAIN_FILE_EXT, files);
    for (auto const & file : files)
    {
      auto const id = base::FilenameWithoutExt(file);
      auto const it = std::find_if(blocks.begin(), blocks.end(), [&](auto const & block) { return block.m_id == id; });
      TEST(it != blocks.end(), (id));
      if (it != blocks.end())
        scanned.push_back(MakeTerrainFile(*it, version));
    }
  }
  storage.OnTerrainScanned(std::vector<terrain::TwmFile>(scanned));
  return scanned;
}

// Records the terrain retry armings without running them (the fire side is covered by
// the production StorageDownloadingPolicy; here only the classification matters).
class RecordingDownloadingPolicy : public DownloadingPolicy
{
public:
  void ScheduleTerrainRetry(storage::CountriesSet const & regions, TProcessFunc const &,
                            bool /* hasNonRetryableFailures */ = false) override
  {
    if (regions.empty())
      return;  // The drain-time counter reset, not an arming.
    ++m_calls;
    m_regions.insert(regions.begin(), regions.end());
  }

  size_t m_calls = 0;
  storage::CountriesSet m_regions;
};

// Keep downloads pending until the test provides their server response.
class PendingTerrainDownloader : public MapFilesDownloader
{
public:
  PendingTerrainDownloader() { SetServersList({"http://test-url/"}); }

  void Remove(CountryId const & id) override
  {
    MapFilesDownloader::Remove(id);
    m_queue.Remove(id);
  }

  void Clear() override
  {
    MapFilesDownloader::Clear();
    m_queue.Clear();
  }

  QueueInterface const & GetQueue() const override { return m_queue; }

  void Start() { m_queue.GetFirstCountry().OnStartDownloading(); }

  void Progress(uint64_t bytesDownloaded)
  {
    auto const & country = m_queue.GetFirstCountry();
    country.OnDownloadProgress(
        {static_cast<int64_t>(bytesDownloaded), static_cast<int64_t>(country.GetDownloadSize())});
  }

  void Complete(std::string const & content)
  {
    auto const country = m_queue.GetFirstCountry();
    {
      FileWriter writer(country.GetFileDownloadPath());
      writer.Write(content.data(), content.size());
    }
    m_queue.PopFront();
    country.OnDownloadFinished(downloader::DownloadStatus::Completed);
  }

  void Fail(downloader::DownloadStatus status)
  {
    auto const country = m_queue.GetFirstCountry();
    m_queue.PopFront();
    country.OnDownloadFinished(status);
  }

  size_t m_requests = 0;

private:
  void Download(QueuedCountry && country) override
  {
    ++m_requests;
    m_queue.Append(std::move(country));
  }

  Queue m_queue;
};

std::string TerrainHeaderContent(terrain::GridBlock const & block = {8, 45, 3, 2}, size_t payloadSize = 0)
{
  tests_support::ScopedFile file("terrain_header.twm", tests_support::ScopedFile::Mode::Create);
  terrain::TwmHeader header;
  auto const rect = block.GetRectMercator();
  header.m_geometries = {{17, 0}};
  header.m_limitLB = PointDToPointU(rect.LeftBottom(), header.m_coordBits);
  header.m_limitRT = PointDToPointU(rect.RightTop(), header.m_coordBits);
  {
    FilesContainerW container(file.GetFullPath());
    auto writer = container.GetWriter(terrain::kHeaderTag);
    header.Serialize(*writer);
    if (payloadSize != 0)
    {
      writer.reset();
      writer = container.GetWriter("test_payload");
      std::string const payload(payloadSize, 't');
      writer->Write(payload.data(), payload.size());
    }
  }
  std::string content;
  FileReader(file.GetFullPath()).ReadAsString(content);
  return content;
}

std::string TerrainGridJson(std::string const & content, bool correctHash = true)
{
  coding::Blake3 hasher;
  hasher.Update(content.data(), content.size());
  auto const hash = correctHash ? hasher.FinalizeToBase64(coding::Blake3::kMwmHashSizeInBytes) : "incorrect";
  return R"({"v": 1, "blocks": [{"id": "N45E008", "sx": 3, "sy": 2, "s": )" + strings::to_string(content.size()) +
         R"(, "h": ")" + hash + R"("}], "mwms": {"Madagascar": ["N45E008"]}})";
}

class StorageTerrainDownloadTest
{
protected:
  WritableDirChanger const m_writableDirChanger{kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir};
  ScopedDownloadQueue const m_guardSettings;
  Platform::ThreadRunner m_threadRunner;
};

UNIT_CLASS_TEST(StorageTerrainDownloadTest, UpdateProgressKeepsMapsAndTerrainInOneBatch)
{
  auto const content = TerrainHeaderContent({8, 45, 3, 2}, 2 * 1024 * 1024);
  tests_support::ScopedFile countries("combined_update.json", R"({"id":"Countries","v":260101,
    "g":[{"id":"Group","g":[{"id":"West","s":100}]}]})");
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, R"({"v":1,"blocks":[
    {"id":"N45E008","sx":3,"sy":2,"s":)" + strings::to_string(content.size()) +
                                                        R"(,"h":"aA"}],"mwms":{"West":["N45E008"]}})");
  tests_support::ScopedDir oldMaps("260100");
  tests_support::ScopedFile oldMap("260100/West.mwm", "old");
  tests_support::ScopedDirCleanup const mapsDir(base::JoinPath(GetPlatform().WritableDir(), "260101"));
  tests_support::ScopedDirCleanup const terrainDir(base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR));
  Storage storage("combined_update.json");
  storage.Init([](auto const &, auto const &) {}, [](auto const &, auto const &) { return false; });
  storage.SetEnabledIntegrityValidationForTesting(false);
  storage.RegisterAllLocalMaps();
  storage.OnTerrainScanned({});
  storage.SetTerrainCallbacks([](auto const &)
  {
    testing::StopEventLoop();
    return true;
  }, {});
  auto downloader = std::make_unique<PendingTerrainDownloader>();
  auto & pending = *downloader;
  storage.SetDownloaderForTesting(std::move(downloader));
  int64_t const total = 100 + content.size();
  Storage::UpdateInfo info;
  TEST(storage.GetUpdateInfo("Group", info), ());
  TEST_EQUAL(info.m_numberOfMwmFilesToUpdate, 1, ());
  TEST_EQUAL(info.m_totalDownloadSizeInBytes, static_cast<uint64_t>(total), ());
  storage.UpdateNode("Group");
  TEST_EQUAL(pending.GetQueue().Count(), 2, ());
  auto const check = [&](int64_t downloaded)
  {
    auto const overall = storage.GetOverallProgress({"West", "West"});
    TEST_EQUAL(overall.m_bytesDownloaded, downloaded, ());
    TEST_EQUAL(overall.m_bytesTotal, total, ());
    for (auto const & id : {CountryId("West"), CountryId("Group"), storage.GetRootId()})
    {
      auto const progress = storage.GetDownloadingProgress(id).GetCombined();
      TEST_EQUAL(progress.m_bytesDownloaded, downloaded, (id));
      TEST_EQUAL(progress.m_bytesTotal, total, (id));
      NodeAttrs attrs;
      storage.GetNodeAttrs(id, attrs);
      TEST_EQUAL(attrs.m_status, NodeStatus::Downloading, (id));
      TEST_EQUAL(attrs.m_downloadSize, static_cast<uint64_t>(total), (id));
      TEST_EQUAL(attrs.m_downloadingProgress.m_bytesDownloaded, downloaded, (id));
      TEST_EQUAL(attrs.m_downloadingProgress.m_bytesTotal, total, (id));
      NodeStatuses status;
      storage.GetNodeStatuses(id, status);
      TEST_EQUAL(status.m_status, attrs.m_status, (id));
    }
  };
  pending.Start();
  pending.Progress(50);
  check(50);
  // Metadata can change while a full-map request is in flight; retain the actual request size.
  storage.SetDiffInfoForTesting({{"West", {10, 260100}}});
  pending.Progress(100);
  pending.Complete(std::string(100, 'm'));
  check(100);
  pending.Start();
  pending.Progress(content.size() / 2);
  check(100 + content.size() / 2);
  pending.Progress(content.size());
  pending.Complete(content);
  check(total);  // Validation still belongs to the same batch.
  testing::RunEventLoop();
  NodeAttrs attrs;
  storage.GetNodeAttrs("West", attrs);
  TEST_EQUAL(attrs.m_status, NodeStatus::OnDisk, ());
  TEST_EQUAL(attrs.m_downloadingProgress.m_bytesDownloaded, attrs.m_downloadingProgress.m_bytesTotal, ());
  TEST(!storage.IsDownloadInProgress(), ());
  TEST(storage.GetUpdateInfo("Group", info), ());
  TEST_EQUAL(info.m_totalDownloadSizeInBytes, 0, ());
}

UNIT_CLASS_TEST(StorageTerrainDownloadTest, FailedDownloadSizeMatchesRetry)
{
  tests_support::ScopedFile countries("retry_sizes.json", R"({"id":"Countries","v":260101,"g":[
    {"id":"Group","g":[{"id":"West","s":100},{"id":"East","s":200},{"id":"Missing","s":300}]},
    {"id":"Other","g":[{"id":"West","s":100}]}]})");
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, R"({"v":1,"blocks":[
    {"id":"N45E008","sx":3,"sy":2,"s":30,"h":"aA"},
    {"id":"N45E011","sx":3,"sy":2,"s":10,"h":"aA"},
    {"id":"N45E014","sx":3,"sy":2,"s":5,"h":"aA"}],
    "mwms":{"West":["N45E008","N45E011"],"East":["N45E008"],"Missing":["N45E014"]}})");
  for (bool const withDiff : {false, true})
  {
    tests_support::ScopedDirCleanup const mapsDir(base::JoinPath(GetPlatform().WritableDir(), "260101"));
    auto downloader = std::make_unique<PendingTerrainDownloader>();
    auto & pending = *downloader;
    Storage storage("retry_sizes.json");
    storage.Init([](auto const &, auto const &) {}, [](auto const &, auto const &) { return false; });
    storage.SetDownloaderForTesting(std::move(downloader));
    storage.OnTerrainScanned({});
    if (withDiff)
      storage.SetDiffInfoForTesting({{"West", {10, 260100}}});
    storage.DownloadNode("West", withDiff /* isUpdate */);
    storage.DownloadTerrain("West");
    // The failed shared block has two interested regions, but retry it only once.
    storage.DownloadTerrain("East");
    TEST_EQUAL(pending.GetQueue().Count(), 3, (withDiff));
    while (!pending.GetQueue().IsEmpty())
      pending.Fail(downloader::DownloadStatus::Failed);
    uint64_t const expectedSize = (withDiff ? 10 : 100) + 40;
    for (auto const & id : {CountryId("West"), CountryId("Group"), storage.GetRootId()})
    {
      NodeAttrs attrs;
      storage.GetNodeAttrs(id, attrs);
      TEST_EQUAL(attrs.m_status, NodeStatus::Error, (id, withDiff));
      TEST_EQUAL(attrs.m_downloadSize, expectedSize, (id, withDiff));
    }
    storage.RetryDownloadNode(storage.GetRootId());
    uint64_t requestedSize = 0;
    pending.GetQueue().ForEachCountry([&](QueuedCountry const & country)
    {
      requestedSize += country.GetDownloadSize();
      TEST_NOT_EQUAL(country.GetCountryId(), "Missing", (withDiff));
    });
    TEST_EQUAL(pending.GetQueue().Count(), 3, (withDiff));
    TEST_EQUAL(requestedSize, expectedSize, (withDiff));
    storage.CancelDownloadNode(storage.GetRootId());
    if (withDiff)
    {
      // A failed diff alone must not include terrain that was never requested.
      storage.DownloadNode("West", true /* isUpdate */);
      pending.Fail(downloader::DownloadStatus::Failed);
      NodeAttrs attrs;
      storage.GetNodeAttrs("Group", attrs);
      TEST_EQUAL(attrs.m_downloadSize, 10, ());
      storage.RetryDownloadNode("Group");
      TEST_EQUAL(pending.GetQueue().Count(), 1, ());
      TEST_EQUAL(static_cast<Queue const &>(pending.GetQueue()).GetFirstCountry().GetDownloadSize(), 10, ());
      storage.CancelDownloadNode(storage.GetRootId());
    }
  }
}

UNIT_CLASS_TEST(StorageTerrainDownloadTest, RoutingDownloadsOnlyMapsAndIgnoresTerrainProgress)
{
  tests_support::ScopedFile countries("routing_maps.json", R"({"id":"Countries","v":260101,
    "g":[{"id":"Group","g":[{"id":"West","s":100},{"id":"East","s":200}]}]})");
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, R"({"v":1,"blocks":[
    {"id":"N45E008","sx":3,"sy":2,"s":30,"h":"aA"},
    {"id":"N45E011","sx":3,"sy":2,"s":10,"h":"aA"}],
    "mwms":{"West":["N45E008","N45E011"],"East":["N45E008"]}})");
  tests_support::ScopedDirCleanup const mapsDir(base::JoinPath(GetPlatform().WritableDir(), "260101"));
  Storage storage("routing_maps.json");
  storage.Init([](auto const &, auto const &) {}, [](auto const &, auto const &) { return false; });
  storage.SetEnabledIntegrityValidationForTesting(false);
  storage.OnTerrainScanned({});
  auto downloader = std::make_unique<PendingTerrainDownloader>();
  auto & pending = *downloader;
  storage.SetDownloaderForTesting(std::move(downloader));
  CountriesVec const maps = {"West", "East", "West"};
  TEST_EQUAL(storage.GetDownloadSize(maps, false /* withTerrain */), 300, ());
  TEST_EQUAL(storage.GetDownloadSize(maps), 340, ());
  storage.DownloadNode("Group", false /* isUpdate */, false /* withTerrain */);
  TEST_EQUAL(pending.GetQueue().Count(), 2, ());
  pending.GetQueue().ForEachCountry([](QueuedCountry const & country)
  { TEST_EQUAL(country.GetFileType(), MapFileType::Map, ()); });
  auto const checkProgress = [&](int64_t downloaded)
  {
    auto const progress = storage.GetOverallProgress(maps, false /* withTerrain */);
    TEST_EQUAL(progress.m_bytesDownloaded, downloaded, ());
    TEST_EQUAL(progress.m_bytesTotal, 300, ());
  };
  pending.Start();
  pending.Progress(50);
  checkProgress(50);
  // An independent terrain request must not inflate routing progress or be cancelled with its maps.
  storage.DownloadTerrain("West");
  TEST_EQUAL(pending.GetQueue().Count(), 4, ());
  TEST_EQUAL(storage.GetOverallProgress(maps).m_bytesTotal, 340, ());
  pending.Complete(std::string(100, 'm'));
  TEST(storage.HasLatestVersion("West"), ());
  NodeStatuses status;
  storage.GetNodeStatuses("West", status);
  TEST_EQUAL(status.m_status, NodeStatus::Downloading, ());
  checkProgress(100);
  pending.Start();
  pending.Progress(100);
  checkProgress(200);
  pending.Complete(std::string(200, 'm'));
  checkProgress(300);
  TEST(storage.HasLatestVersion("East"), ());
  TEST(storage.IsDownloadInProgress(), ());
  storage.CancelDownloadNode("Group", false /* withTerrain */);
  TEST_EQUAL(pending.GetQueue().Count(), 2, ());
  storage.DownloadNode("West", false /* isUpdate */, false /* withTerrain */);
  TEST_EQUAL(pending.GetQueue().Count(), 2, ());
  storage.CancelTerrain("Group");
  TEST(!storage.IsDownloadInProgress(), ());
  checkProgress(300);
  TEST_EQUAL(storage.GetDownloadSize(maps, false /* withTerrain */), 0, ());
  TEST_EQUAL(storage.GetDownloadSize(maps), 40, ());
}

UNIT_CLASS_TEST(StorageTerrainDownloadTest, RestorePreservesMapAndTerrainRequests)
{
  tests_support::ScopedFile countries("restore_maps.json", R"({"id":"Countries","v":260101,
    "g":[{"id":"West","s":100}]})");
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, R"({"v":1,"blocks":[
    {"id":"N45E008","sx":3,"sy":2,"s":30,"h":"aA"}],"mwms":{"West":["N45E008"]}})");
  for (bool const withTerrain : {false, true})
  {
    tests_support::ScopedDirCleanup const mapsDir(base::JoinPath(GetPlatform().WritableDir(), "260101"));
    tests_support::ScopedDirCleanup const terrainDir(base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR));
    std::map<std::string, std::string> saved;
    {
      Storage storage("restore_maps.json");
      storage.OnTerrainScanned({});
      storage.SetDownloaderForTesting(std::make_unique<PendingTerrainDownloader>());
      storage.DownloadNode("West", false /* isUpdate */, withTerrain);
      for (auto const * key : {"DownloadQueue", "TerrainDownloadQueue", "TerrainRequestedRegions"})
        settings::TryGet(key, saved[key]);
      TEST_EQUAL(saved["DownloadQueue"], "West", (withTerrain));
      TEST_EQUAL(saved["TerrainDownloadQueue"], withTerrain ? "West" : "", (withTerrain));
    }
    for (bool const scanFirst : {false, true})
    {
      for (auto const & [key, value] : saved)
        settings::Set(key, value);
      Storage restored("restore_maps.json");
      auto downloader = std::make_unique<PendingTerrainDownloader>();
      auto const & queue = downloader->GetQueue();
      restored.SetDownloaderForTesting(std::move(downloader));
      if (scanFirst)
        restored.OnTerrainScanned({});
      restored.RestoreDownloadQueue();
      if (!scanFirst)
      {
        TEST(queue.IsEmpty(), (withTerrain));
        restored.OnTerrainScanned({});
      }
      TEST_EQUAL(queue.Count(), withTerrain ? 2u : 1u, (withTerrain, scanFirst));
      TEST_EQUAL(static_cast<Queue const &>(queue).GetFirstCountry().GetFileType(), MapFileType::Map, ());
      TEST_EQUAL(queue.Contains("N45E008"), withTerrain, (scanFirst));
      restored.CancelDownloadNode(restored.GetRootId());
    }
  }
}

UNIT_CLASS_TEST(StorageTerrainDownloadTest, ManualRetryPreservesMapAndTerrainRequests)
{
  tests_support::ScopedFile countries("manual_retry_maps.json", R"({"id":"Countries","v":260101,
    "g":[{"id":"West","s":100}]})");
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, R"({"v":1,"blocks":[
    {"id":"N45E008","sx":3,"sy":2,"s":30,"h":"aA"}],"mwms":{"West":["N45E008"]}})");
  for (bool const withTerrain : {false, true})
  {
    tests_support::ScopedDirCleanup const mapsDir(base::JoinPath(GetPlatform().WritableDir(), "260101"));
    Storage storage("manual_retry_maps.json");
    storage.Init([](auto const &, auto const &) {}, [](auto const &, auto const &) { return false; });
    storage.OnTerrainScanned({});
    auto downloader = std::make_unique<PendingTerrainDownloader>();
    auto & pending = *downloader;
    storage.SetDownloaderForTesting(std::move(downloader));
    storage.DownloadNode("West", false /* isUpdate */, withTerrain);
    while (!pending.GetQueue().IsEmpty())
      pending.Fail(downloader::DownloadStatus::FileNotFound);
    for (auto const & id : {CountryId("West"), storage.GetRootId()})
    {
      NodeAttrs attrs;
      storage.GetNodeAttrs(id, attrs);
      TEST_EQUAL(attrs.m_status, NodeStatus::Error, (withTerrain, id));
      TEST_EQUAL(attrs.m_downloadSize, withTerrain ? 130u : 100u, (id));
    }
    storage.RetryDownloadNode(storage.GetRootId());
    TEST_EQUAL(pending.GetQueue().Count(), withTerrain ? 2u : 1u, (withTerrain));
    TEST_EQUAL(pending.GetQueue().Contains("N45E008"), withTerrain, ());
    TEST_EQUAL(storage.GetOverallProgress({"West"}, false /* withTerrain */).m_bytesTotal, 100, (withTerrain));
    storage.CancelDownloadNode(storage.GetRootId());
  }
}

UNIT_CLASS_TEST(StorageTerrainDownloadTest, RoutingMapRetryDoesNotDownloadTerrain)
{
  class RetryPolicy : public DownloadingPolicy
  {
  public:
    void ScheduleRetry(CountriesSet const &, TProcessFunc const & retry) override { m_retry = retry; }
    TProcessFunc m_retry;
  } policy;
  tests_support::ScopedFile countries("routing_retry.json", R"({"id":"Countries","v":260101,
    "g":[{"id":"West","s":100}]})");
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, R"({"v":1,"blocks":[
    {"id":"N45E008","sx":3,"sy":2,"s":30,"h":"aA"}],"mwms":{"West":["N45E008"]}})");
  tests_support::ScopedDirCleanup const mapsDir(base::JoinPath(GetPlatform().WritableDir(), "260101"));
  Storage storage("routing_retry.json");
  storage.Init([](auto const &, auto const &) {}, [](auto const &, auto const &) { return false; });
  storage.SetDownloadingPolicy(&policy);
  storage.OnTerrainScanned({});
  auto downloader = std::make_unique<PendingTerrainDownloader>();
  auto & pending = *downloader;
  storage.SetDownloaderForTesting(std::move(downloader));
  storage.DownloadNode("West", false /* isUpdate */, false /* withTerrain */);
  pending.Fail(downloader::DownloadStatus::Failed);
  TEST(static_cast<bool>(policy.m_retry), ());
  policy.m_retry({"West"});
  TEST_EQUAL(pending.GetQueue().Count(), 1, ());
  pending.GetQueue().ForEachCountry([](QueuedCountry const & country)
  { TEST_EQUAL(country.GetFileType(), MapFileType::Map, ()); });
  storage.CancelDownloadNode("West", false /* withTerrain */);
}

UNIT_CLASS_TEST(StorageTerrainDownloadTest, MapAutoRetryWhileTerrainRetryIsActive)
{
  class RetryPolicy : public DownloadingPolicy
  {
  public:
    void ScheduleRetry(CountriesSet const &, TProcessFunc const & retry) override { m_mapRetry = retry; }
    void ScheduleTerrainRetry(CountriesSet const & regions, TProcessFunc const & retry,
                              bool /* hasNonRetryableFailures */ = false) override
    {
      if (!regions.empty())
        m_terrainRetry = retry;
    }
    TProcessFunc m_mapRetry;
    TProcessFunc m_terrainRetry;
  } policy;
  tests_support::ScopedFile countries("concurrent_retry.json", R"({"id":"Countries","v":260101,
    "g":[{"id":"West","s":100}]})");
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, R"({"v":1,"blocks":[
    {"id":"N45E008","sx":3,"sy":2,"s":30,"h":"aA"}],"mwms":{"West":["N45E008"]}})");
  tests_support::ScopedDirCleanup const mapsDir(base::JoinPath(GetPlatform().WritableDir(), "260101"));
  tests_support::ScopedDirCleanup const terrainDir(base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR));
  Storage storage("concurrent_retry.json");
  storage.Init([](auto const &, auto const &) {}, [](auto const &, auto const &) { return false; });
  storage.SetDownloadingPolicy(&policy);
  storage.OnTerrainScanned({});
  auto downloader = std::make_unique<PendingTerrainDownloader>();
  auto & pending = *downloader;
  storage.SetDownloaderForTesting(std::move(downloader));
  storage.DownloadNode("West");
  pending.Fail(downloader::DownloadStatus::Failed);
  pending.Fail(downloader::DownloadStatus::Failed);
  TEST(static_cast<bool>(policy.m_mapRetry), ());
  TEST(static_cast<bool>(policy.m_terrainRetry), ());

  // Terrain's timer can fire first and hide the failed map in the combined row status.
  auto const terrainRetry = policy.m_terrainRetry;
  terrainRetry({"West"});
  NodeStatuses status;
  storage.GetNodeStatuses("West", status);
  TEST_EQUAL(status.m_status, NodeStatus::Downloading, ());
  TEST_EQUAL(status.m_error, NodeErrorCode::NoError, ());
  auto const mapRetry = policy.m_mapRetry;
  mapRetry({"West"});
  TEST_EQUAL(pending.GetQueue().Count(), 2, ());
  TEST(pending.GetQueue().Contains("West"), ());
  TEST(pending.GetQueue().Contains("N45E008"), ());
  storage.CancelDownloadNode(storage.GetRootId());
}

UNIT_CLASS_TEST(StorageTerrainDownloadTest, TerrainScanNotifiesSharedAncestorsOnce)
{
  tests_support::ScopedFile countries("scan_notifications.json", R"({"id":"Countries","v":260101,
    "g":[{"id":"Group","g":[{"id":"West","s":100},{"id":"East","s":100}]}]})");
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, R"({"v":1,"blocks":[
    {"id":"N45E008","sx":3,"sy":2,"s":30,"h":"aA"}],
    "mwms":{"West":["N45E008"],"East":["N45E008"]}})");
  Storage storage("scan_notifications.json");
  std::map<CountryId, size_t> notifications;
  auto const slot = storage.Subscribe([&](CountryId const & id) { ++notifications[id]; },
                                      [](CountryId const &, storage::Progress const &) {});
  storage.OnTerrainScanned({});
  TEST_EQUAL(notifications["West"], 1, ());
  TEST_EQUAL(notifications["East"], 1, ());
  TEST_EQUAL(notifications["Group"], 1, ());
  for (auto const & [id, count] : notifications)
    TEST_EQUAL(count, 1, (id));
  storage.Unsubscribe(slot);
}

UNIT_CLASS_TEST(StorageTerrainDownloadTest, PublishDownloadedBlock)
{
  auto const content = TerrainHeaderContent();
  tests_support::ScopedDir customDataDir("custom");
  // Both the network completion and a pre-existing .ready go through validation and registration.
  for (bool const readyOnDisk : {false, true})
  {
    for (auto const & [correctHash, validate, acceptRegistration, dataDir] :
         {std::tuple{true, true, true, std::string{}},
          {false, true, true, std::string{}},
          {false, false, true, std::string{}},
          {true, true, false, std::string{}},
          {true, true, true, std::string{"custom"}}})
    {
      auto const root =
          dataDir.empty() ? GetPlatform().WritableDir() : base::JoinPath(GetPlatform().WritableDir(), dataDir);
      tests_support::ScopedDirCleanup const terrainDir(base::JoinPath(root, TERRAIN_DIR));
      tests_support::ScopedFile grid(TERRAIN_GRID_FILE, TerrainGridJson(content, correctHash));
      Storage storage(COUNTRIES_FILE, dataDir);
      storage.OnTerrainScanned({});
      storage.SetEnabledIntegrityValidationForTesting(validate);
      auto downloader = std::make_unique<PendingTerrainDownloader>();
      auto & pending = *downloader;
      storage.SetDownloaderForTesting(std::move(downloader));
      size_t registrations = 0;
      std::string const finalPath = base::JoinPath(root, TERRAIN_DIR, "1", "N45E008.twm");
      auto const readyPath = finalPath + READY_FILE_EXTENSION;
      bool const valid = correctHash || !validate;
      storage.SetTerrainCallbacks([&](terrain::TwmFile const & file)
      {
        ++registrations;
        TEST(valid, (readyOnDisk));
        TEST_EQUAL(file.m_id, "N45E008", ());
        TEST_EQUAL(file.m_version, 1, ());
        TEST_EQUAL(file.m_path, finalPath, ());
        TEST(Platform::IsFileExistsByFullPath(finalPath), ());
        TEST(!Platform::IsFileExistsByFullPath(readyPath), ());
        return acceptRegistration;
      }, {});
      storage.Subscribe([&](CountryId const & id)
      {
        if (id != "Madagascar")
          return;
        auto const status = storage.GetTerrainAttrs(id).m_status;
        if (status == Storage::TerrainStatus::OnDisk || status == Storage::TerrainStatus::Failed)
          testing::StopEventLoop();
      }, [](auto const &, auto const &) {});
      if (readyOnDisk)
      {
        TEST(Platform::MkDirRecursively(base::GetDirectory(finalPath)), ());
        FileWriter writer(readyPath);
        writer.Write(content.data(), content.size());
      }
      storage.DownloadTerrain("Madagascar");
      TEST_EQUAL(pending.m_requests, readyOnDisk ? 0 : 1, ());
      if (!readyOnDisk)
        pending.Complete(content);
      TEST(storage.IsDownloadInProgress(), (readyOnDisk));
      testing::RunEventLoop();
      TEST(!storage.IsDownloadInProgress(), (readyOnDisk));

      TEST_EQUAL(registrations, valid ? 1 : 0, (readyOnDisk, correctHash, validate, acceptRegistration, dataDir));
      TEST_EQUAL(storage.GetTerrainAttrs("Madagascar").m_status,
                 valid && acceptRegistration ? Storage::TerrainStatus::OnDisk : Storage::TerrainStatus::Failed,
                 (readyOnDisk, correctHash, validate, acceptRegistration, dataDir));
      TEST(!Platform::IsFileExistsByFullPath(readyPath), ());
      TEST_EQUAL(Platform::IsFileExistsByFullPath(finalPath), valid, ());

      // Startup scanning must use the same directory as publishing and resuming.
      auto const scanned = storage.ScanTerrainFiles();
      TEST_EQUAL(scanned.size(), valid ? 1 : 0, (readyOnDisk, correctHash, validate, acceptRegistration, dataDir));
      for (auto const & file : scanned)
        TEST_EQUAL(file.m_path, finalPath, (dataDir));
    }
  }
}

UNIT_CLASS_TEST(StorageTerrainDownloadTest, MapValidationKeepsProgress)
{
  std::string const content(2 * 1024 * 1024, 'm');
  coding::Blake3 hasher;
  hasher.Update(content.data(), content.size());
  auto const hash = hasher.FinalizeToBase64(coding::Blake3::kMwmHashSizeInBytes);
  auto downloader = std::make_unique<PendingTerrainDownloader>();
  auto & pending = *downloader;
  Storage storage(R"({"id": "Countries", "v": 260101, "g": [{"id": "West", "s": 2097152, "h": ")" + hash + R"("}]})",
                  std::move(downloader));
  storage.Init([](CountryId const &, LocalFilePtr const) { testing::StopEventLoop(); },
               [](CountryId const &, LocalFilePtr const) { return false; });
  storage.DownloadNode("West");
  pending.Start();
  pending.Progress(content.size());
  pending.Complete(content);

  // The downloader removes its request before the File thread validates it and
  // the GUI thread publishes it. That gap must retain progress and reject a retry.
  TEST(pending.GetQueue().IsEmpty(), ());
  TEST(storage.IsDownloadInProgress(), ());
  NodeAttrs attrs;
  storage.GetNodeAttrs("West", attrs);
  TEST_EQUAL(attrs.m_status, NodeStatus::Downloading, ());
  TEST_EQUAL(attrs.m_downloadingProgress.m_bytesDownloaded, static_cast<int64_t>(content.size()), ());
  TEST_EQUAL(attrs.m_downloadingProgress.m_bytesTotal, static_cast<int64_t>(content.size()), ());
  TEST_EQUAL(storage.GetDownloadSize({"West"}), 0, ());
  storage.DownloadNode("West");
  TEST_EQUAL(pending.m_requests, 1, ());

  testing::RunEventLoop();
  storage.GetNodeAttrs("West", attrs);
  TEST_EQUAL(attrs.m_status, NodeStatus::OnDisk, ());
  TEST_EQUAL(attrs.m_downloadingProgress.m_bytesDownloaded, static_cast<int64_t>(content.size()), ());
  TEST_EQUAL(attrs.m_downloadingProgress.m_bytesTotal, static_cast<int64_t>(content.size()), ());
  TEST(!storage.IsDownloadInProgress(), ());
}

UNIT_CLASS_TEST(StorageTerrainDownloadTest, CancelMapValidationNotifies)
{
  std::string const content(2 * 1024 * 1024, 'm');
  coding::Blake3 hasher;
  hasher.Update(content.data(), content.size());
  auto const hash = hasher.FinalizeToBase64(coding::Blake3::kMwmHashSizeInBytes);
  auto downloader = std::make_unique<PendingTerrainDownloader>();
  auto & pending = *downloader;
  Storage storage(R"({"id": "Countries", "v": 260101, "g": [{"id": "West", "s": 2097152, "h": ")" + hash + R"("}]})",
                  std::move(downloader));
  storage.Init([](CountryId const &, LocalFilePtr const) {},
               [](CountryId const &, LocalFilePtr const) { return false; });
  storage.DownloadNode("West");
  pending.Start();
  pending.Progress(content.size());
  pending.Complete(content);

  std::vector<Status> cancellationStatuses;
  auto const slot = storage.Subscribe([&](CountryId const & id)
  {
    if (id == "West")
      cancellationStatuses.push_back(storage.CountryStatusEx(id));
  }, [](CountryId const &, storage::Progress const &) {});
  storage.CancelDownloadNode("West");
  auto const downloadingAfterCancel = storage.IsDownloadInProgress();
  storage.Unsubscribe(slot);

  // The request has left the downloader, but cancelling its validation state
  // still changes the UI status. Drain the already scheduled validator first.
  GetPlatform().RunTask(Platform::Thread::File,
                        [] { GetPlatform().RunTask(Platform::Thread::Gui, [] { testing::StopEventLoop(); }); });
  testing::RunEventLoop();
  TEST(!downloadingAfterCancel, ());
  TEST_EQUAL(cancellationStatuses.size(), 1, ());
  TEST_EQUAL(cancellationStatuses.front(), Status::NotDownloaded, ());
}

UNIT_CLASS_TEST(StorageTerrainDownloadTest, MapValidationKeepsGroupProgress)
{
  std::string const content(2 * 1024 * 1024, 'm');
  coding::Blake3 hasher;
  hasher.Update(content.data(), content.size());
  auto const hash = hasher.FinalizeToBase64(coding::Blake3::kMwmHashSizeInBytes);
  auto downloader = std::make_unique<PendingTerrainDownloader>();
  auto & pending = *downloader;
  Storage storage(R"({"id": "Countries", "v": 260101, "g": [{"id": "Group", "g": [
    {"id": "West", "s": 2097152, "h": ")" +
                      hash + R"("},
    {"id": "East", "s": 2097152, "h": ")" +
                      hash + R"("},
    {"id": "Missing", "s": 2097152}]}]})",
                  std::move(downloader));
  std::vector<downloader::Progress> progressAtRegistration;
  storage.Init([&](CountryId const &, LocalFilePtr const)
  {
    NodeAttrs attrs;
    storage.GetNodeAttrs("Group", attrs);
    progressAtRegistration.push_back(attrs.m_downloadingProgress);
    if (progressAtRegistration.size() == 2)
      testing::StopEventLoop();
  }, [](CountryId const &, LocalFilePtr const) { return false; });
  for (auto const * id : {"West", "East"})
  {
    storage.DownloadNode(id);
    pending.Start();
    pending.Progress(content.size());
    pending.Complete(content);
  }

  // Both requests have left the downloader while validation is pending. Publishing
  // the first map must retain its contribution until the second map is published.
  TEST(pending.GetQueue().IsEmpty(), ());
  TEST(storage.IsDownloadInProgress(), ());
  testing::RunEventLoop();
  TEST_EQUAL(progressAtRegistration.size(), 2, ());
  for (auto const & progress : progressAtRegistration)
  {
    TEST_EQUAL(progress.m_bytesDownloaded, static_cast<int64_t>(2 * content.size()), ());
    TEST_EQUAL(progress.m_bytesTotal, static_cast<int64_t>(2 * content.size()), ());
  }
  TEST(!storage.IsDownloadInProgress(), ());
}

UNIT_CLASS_TEST(StorageTerrainDownloadTest, CancelTerrainValidationFinishesBatch)
{
  tests_support::ScopedFile countries("terrain_cancel_validation_countries.json", R"({"id": "Countries", "v": 260101,
    "g": [{"id": "West", "s": 2097152}, {"id": "Missing", "s": 2097152}]})");
  auto const terrain = TerrainHeaderContent();
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, R"({"v": 1, "blocks": [{"id": "N45E008", "sx": 3, "sy": 2, "s": )" +
                                                        strings::to_string(terrain.size()) +
                                                        R"(, "h": "aA"}], "mwms": {"West": ["N45E008"]}})");
  Storage storage("terrain_cancel_validation_countries.json");
  storage.Init([](CountryId const &, LocalFilePtr const) {},
               [](CountryId const &, LocalFilePtr const) { return false; });
  storage.SetEnabledIntegrityValidationForTesting(false);
  storage.OnTerrainScanned({});
  auto downloader = std::make_unique<PendingTerrainDownloader>();
  auto & pending = *downloader;
  storage.SetDownloaderForTesting(std::move(downloader));
  storage.DownloadNode("West");
  storage.DownloadTerrain("West");
  pending.Start();
  pending.Complete(std::string(2 * 1024 * 1024, 'm'));
  pending.Start();
  pending.Complete(terrain);
  TEST(storage.IsDownloadInProgress(), ());
  TEST(pending.GetQueue().IsEmpty(), ());

  // The Desktop terrain column calls this directly, without cancelling maps.
  storage.CancelTerrain("West");
  TEST(!storage.IsDownloadInProgress(), ());
  NodeAttrs attrs;
  storage.GetNodeAttrs(storage.GetRootId(), attrs);

  // Drain the already scheduled validation callback before destroying Storage.
  GetPlatform().RunTask(Platform::Thread::File,
                        [] { GetPlatform().RunTask(Platform::Thread::Gui, [] { testing::StopEventLoop(); }); });
  testing::RunEventLoop();
  TEST_EQUAL(attrs.m_downloadingProgress.m_bytesDownloaded, 0, ());
  TEST_EQUAL(attrs.m_downloadingProgress.m_bytesTotal, 0, ());
  TEST_EQUAL(storage.GetTerrainAttrs("West").m_status, Storage::TerrainStatus::NotDownloaded, ());
}

UNIT_TEST(Storage_DiffFallbackReplacesActiveDownload)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  auto downloader = std::make_unique<PendingTerrainDownloader>();
  auto & pending = *downloader;
  Storage storage(R"({"id": "Countries", "v": 260101,
    "g": [{"id": "West", "s": 2097152}, {"id": "East", "s": 2097152}]})",
                  std::move(downloader));
  storage.Init([](CountryId const &, LocalFilePtr const) {},
               [](CountryId const &, LocalFilePtr const) { return false; });
  storage.SetDiffInfoForTesting({{"West", {1024, 260100}}, {"East", {2048, 260100}}});
  storage.DownloadCountry("West", MapFileType::Diff);
  storage.DownloadCountry("East", MapFileType::Diff);
  // A background downloader can start several requests before a missing diff
  // invalidates the scheme and replaces the remaining active diffs with maps.
  pending.GetQueue().ForEachCountry([](QueuedCountry const & country) { country.OnStartDownloading(); });
  pending.Fail(downloader::DownloadStatus::FileNotFound);

  TEST_EQUAL(pending.m_requests, 3, ());
  TEST(pending.GetQueue().Contains("East"), ());
  pending.GetQueue().ForEachCountry([](QueuedCountry const & country)
  {
    TEST_EQUAL(country.GetCountryId(), "East", ());
    TEST_EQUAL(country.GetFileType(), MapFileType::Map, ());
  });
  TEST_EQUAL(storage.CountryStatusEx("East"), Status::InQueue, ());
  storage.CancelDownloadNode(storage.GetRootId());
  TEST(!storage.IsDownloadInProgress(), ());
}

UNIT_TEST(Storage_TerrainDeregisterKeepsQueuedDirectory)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE,
                                 R"({"v": 1, "blocks": [{"id": "N45E008", "sx": 3, "sy": 2, "s": 1, "h": "aA"},
                             {"id": "N45E011", "sx": 2, "sy": 2, "s": 1, "h": "aA"}],
          "mwms": {"Madagascar": ["N45E008", "N45E011"]}})");
  tests_support::ScopedDir terrainDir(TERRAIN_DIR);
  tests_support::ScopedDir versionDir(terrainDir, "1");
  tests_support::ScopedFile installed(base::JoinPath(versionDir.GetRelativePath(), "N45E008.twm"), "twm");
  terrain::TwmFile const file{"N45E008", 1, installed.GetFullPath(), terrain::GridBlock{8, 45, 3, 2}.GetRectMercator()};
  Storage storage;
  storage.OnTerrainScanned({file});
  auto downloader = std::make_unique<PendingTerrainDownloader>();
  auto const & queue = downloader->GetQueue();
  storage.SetDownloaderForTesting(std::move(downloader));
  storage.DownloadTerrain("Madagascar");
  TEST_EQUAL(queue.Count(), 1, ());
  TEST(queue.Contains("N45E011"), ());

  // A reader can release the last installed file while the next block is still queued.
  storage.OnTerrainFileDeregistered(file);
  TEST(!installed.Exists(), ());
  installed.Reset();
  TEST(versionDir.Exists(), ());
  TEST(terrainDir.Exists(), ());
  tests_support::ScopedFile next(base::JoinPath(versionDir.GetRelativePath(), "N45E011.twm.ready.downloading"), "part");
  TEST(next.Exists(), ());
}

UNIT_TEST(Storage_TerrainRestoreQueuedWithoutArtifacts)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, TerrainGridJson("twm"));
  Storage const mapInfo;
  for (bool const mapOnDisk : {false, true})
  {
    std::unique_ptr<tests_support::ScopedDir> mapsDir;
    std::unique_ptr<tests_support::ScopedFile> map;
    if (mapOnDisk)
    {
      mapsDir = std::make_unique<tests_support::ScopedDir>(strings::to_string(mapInfo.GetCurrentDataVersion()));
      map =
          std::make_unique<tests_support::ScopedFile>(*mapsDir, platform::CountryFile("Madagascar"), MapFileType::Map);
      ResizeToRemote(*map, mapInfo, "Madagascar");
    }
    std::string snapshot;
    {
      Storage storage;
      storage.RegisterAllLocalMaps();
      storage.OnTerrainScanned({});
      storage.SetDownloaderForTesting(std::make_unique<PendingTerrainDownloader>());
      storage.DownloadTerrain("Madagascar");
      TEST(settings::Get("TerrainDownloadQueue", snapshot), ());
      TEST_EQUAL(snapshot, "Madagascar", (mapOnDisk));
      std::string mapQueue;
      settings::TryGet("DownloadQueue", mapQueue);
      TEST(mapQueue.empty(), (mapOnDisk, mapQueue));
      Platform::TFilesWithType files;
      Platform::GetFilesByType(base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR, "1"), Platform::Regular, files);
      TEST(files.empty(), ());
    }
    auto const savedQueue = snapshot;
    for (bool const scanFirst : {false, true})
    {
      settings::Set("TerrainDownloadQueue", savedQueue);
      Storage storage;
      storage.RegisterAllLocalMaps();
      auto downloader = std::make_unique<PendingTerrainDownloader>();
      auto const & queue = downloader->GetQueue();
      storage.SetDownloaderForTesting(std::move(downloader));
      if (scanFirst)
        storage.OnTerrainScanned({});
      storage.RestoreDownloadQueue();
      if (!scanFirst)
      {
        TEST(queue.IsEmpty(), ());
        storage.OnTerrainScanned({});
      }
      TEST_EQUAL(queue.Count(), 1, (mapOnDisk, scanFirst));
      TEST(queue.Contains("N45E008"), (mapOnDisk, scanFirst));
      TEST_EQUAL(storage.GetTerrainAttrs("Madagascar").m_status, Storage::TerrainStatus::Downloading,
                 (mapOnDisk, scanFirst));
      storage.CancelTerrain("Madagascar");
      TEST(queue.IsEmpty(), (mapOnDisk, scanFirst));
      TEST(settings::Get("TerrainDownloadQueue", snapshot), ());
      TEST(snapshot.empty(), (mapOnDisk, scanFirst, snapshot));
    }
  }
}

UNIT_TEST(Storage_TerrainDeleteBeforeScanCancelsSavedDownload)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE,
                                 R"({"v": 1, "blocks": [{"id": "N45E008", "sx": 3, "sy": 2, "s": 1, "h": "aA"},
                             {"id": "N45E011", "sx": 2, "sy": 2, "s": 1, "h": "aA"}],
          "mwms": {"Madagascar": ["N45E008"], "Antarctica": ["N45E011"]}})");
  for (bool const groupOwner : {false, true})
  {
    for (bool const restoreFirst : {false, true})
    {
      Storage storage;
      auto downloader = std::make_unique<PendingTerrainDownloader>();
      auto const & queue = downloader->GetQueue();
      storage.SetDownloaderForTesting(std::move(downloader));
      settings::Set("TerrainDownloadQueue", groupOwner ? storage.GetRootId() : "Madagascar;Antarctica");
      if (restoreFirst)
        storage.RestoreDownloadQueue();
      storage.DeleteTerrain("Madagascar");
      storage.OnTerrainScanned({});
      if (!restoreFirst)
        storage.RestoreDownloadQueue();

      TEST_EQUAL(queue.Count(), 1, (groupOwner, restoreFirst));
      TEST(queue.Contains("N45E011"), (groupOwner, restoreFirst));
      TEST_EQUAL(storage.GetTerrainAttrs("Madagascar").m_status, Storage::TerrainStatus::NotDownloaded, ());
      std::string saved;
      TEST(settings::Get("TerrainDownloadQueue", saved), ());
      TEST_EQUAL(saved, "Antarctica", (groupOwner, restoreFirst));
      storage.CancelTerrain("Antarctica");
    }
  }
}

UNIT_TEST(Storage_TerrainMapCancellationBeforeRestorePreservesOtherOwners)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE,
                                 R"({"v": 1, "blocks": [{"id": "N45E008", "sx": 3, "sy": 2, "s": 1, "h": "aA"},
                             {"id": "N45E011", "sx": 2, "sy": 2, "s": 1, "h": "aA"}],
          "mwms": {"Madagascar": ["N45E008"], "Antarctica": ["N45E011"]}})");
  for (bool const scanFirst : {false, true})
  {
    for (bool const groupOwner : {false, true})
    {
      Storage storage;
      auto downloader = std::make_unique<PendingTerrainDownloader>();
      auto const & queue = downloader->GetQueue();
      storage.SetDownloaderForTesting(std::move(downloader));
      settings::Set("TerrainDownloadQueue", groupOwner ? storage.GetRootId() : "Madagascar;Antarctica");
      if (scanFirst)
        storage.OnTerrainScanned({});
      else
        storage.RestoreDownloadQueue();  // Deferred until the scan completes.

      // Both map enqueue and cancellation save the queues before terrain is restored.
      storage.DownloadNode("Uruguay", true /* isUpdate */);
      TEST(queue.Contains("Uruguay"), ());
      storage.CancelDownloadNode("Uruguay");
      storage.DeleteTerrain("Madagascar");
      std::string saved;
      TEST(settings::Get("TerrainDownloadQueue", saved), ());
      TEST_EQUAL(saved, "Antarctica", (scanFirst, groupOwner));

      if (scanFirst)
        storage.RestoreDownloadQueue();
      else
        storage.OnTerrainScanned({});
      TEST_EQUAL(queue.Count(), 1, (scanFirst, groupOwner));
      TEST(queue.Contains("N45E011"), (scanFirst, groupOwner));
      storage.CancelTerrain("Antarctica");
      TEST(settings::Get("TerrainDownloadQueue", saved), ());
      TEST(saved.empty(), (scanFirst, groupOwner, saved));
      storage.RestoreDownloadQueue();
      TEST(queue.IsEmpty(), ());
    }
  }
}

UNIT_TEST(Storage_TerrainClearBeforeRestoreDiscardsSavedQueue)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, TerrainGridJson("twm"));
  settings::Set("TerrainDownloadQueue", std::string{"Madagascar"});
  Storage storage;
  storage.SetDownloaderForTesting(std::make_unique<PendingTerrainDownloader>());
  storage.Clear();
  storage.OnTerrainScanned({});
  storage.RestoreDownloadQueue();
  TEST_EQUAL(storage.GetTerrainAttrs("Madagascar").m_status, Storage::TerrainStatus::NotDownloaded, ());
  std::string saved;
  TEST(settings::Get("TerrainDownloadQueue", saved), ());
  TEST(saved.empty(), (saved));
}

UNIT_TEST(Storage_TerrainOutOfDateStatus)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  Storage storage;
  ScanTerrain(storage);

  TEST_EQUAL(storage.GetTerrainAttrs("Madagascar").m_status, Storage::TerrainStatus::NotDownloaded, ());

  int64_t version = 0;
  std::vector<Storage::TerrainBlock> blocks;
  std::map<CountryId, std::vector<uint32_t>> coverage;
  std::string content;
  GetPlatform().GetReader(TERRAIN_GRID_FILE)->ReadAsString(content);
  Storage::ParseTwmGridJson(content, version, blocks, coverage);
  auto const & block = blocks[coverage.at("Madagascar").front()];
  tests_support::ScopedDir terrainDir(TERRAIN_DIR);
  tests_support::ScopedDir versionDir(terrainDir, strings::to_string(block.m_version - 1));
  tests_support::ScopedFile blockFile(base::JoinPath(versionDir.GetRelativePath(), block.m_id + TERRAIN_FILE_EXT),
                                      "twm");
  storage.OnTerrainScanned({MakeTerrainFile(block, block.m_version - 1)});
  TEST_EQUAL(storage.GetTerrainAttrs("Madagascar").m_status, Storage::TerrainStatus::OnDiskOutOfDate, ());
}

UNIT_TEST(Storage_TerrainScanRegistration)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  int64_t version = 0;
  std::vector<Storage::TerrainBlock> blocks;
  std::map<CountryId, std::vector<uint32_t>> coverage;
  std::string content;
  GetPlatform().GetReader(TERRAIN_GRID_FILE)->ReadAsString(content);
  Storage::ParseTwmGridJson(content, version, blocks, coverage);
  auto const owner =
      std::find_if(coverage.begin(), coverage.end(), [](auto const & entry) { return entry.second.size() == 1; });
  TEST(owner != coverage.end(), ());
  auto const & block = blocks[owner->second.front()];

  tests_support::ScopedDir terrainDir(TERRAIN_DIR);
  tests_support::ScopedDir currentDir(terrainDir, strings::to_string(block.m_version));
  tests_support::ScopedDir olderDir(terrainDir, strings::to_string(block.m_version - 1));
  tests_support::ScopedFile currentFile(base::JoinPath(currentDir.GetRelativePath(), block.m_id + TERRAIN_FILE_EXT),
                                        "twm");
  tests_support::ScopedFile olderFile(base::JoinPath(olderDir.GetRelativePath(), block.m_id + TERRAIN_FILE_EXT), "twm");
  auto const current = MakeTerrainFile(block, block.m_version);
  auto const older = MakeTerrainFile(block, block.m_version - 1);

  Storage storage;
  std::vector<int64_t> registered;
  storage.SetTerrainCallbacks([&](terrain::TwmFile const & file)
  {
    registered.push_back(file.m_version);
    return true;
  }, {});
  storage.OnTerrainScanned({older, current});
  TEST_EQUAL(registered, (std::vector<int64_t>{block.m_version}), ());
  TEST_EQUAL(storage.GetTerrainAttrs(owner->first).m_status, Storage::TerrainStatus::OnDisk, ());
  TEST(currentFile.Exists(), ());
  TEST(!olderFile.Exists(), ());
  olderFile.Reset();

  // A file rejected by the registry cannot make the region appear downloaded.
  Storage rejected;
  size_t attempts = 0;
  rejected.SetTerrainCallbacks([&](terrain::TwmFile const & file)
  {
    ++attempts;
    TEST_EQUAL(file.m_id, block.m_id, ());
    return false;
  }, {});
  rejected.OnTerrainScanned({current});
  TEST_EQUAL(attempts, 1, ());
  TEST_EQUAL(rejected.GetTerrainAttrs(owner->first).m_status, Storage::TerrainStatus::NotDownloaded, ());
  if (!currentFile.Exists())
    currentFile.Reset();
}

UNIT_TEST(Storage_TerrainDeleteDuringScan)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  int64_t version = 0;
  std::vector<Storage::TerrainBlock> blocks;
  std::map<CountryId, std::vector<uint32_t>> coverage;
  std::string content;
  GetPlatform().GetReader(TERRAIN_GRID_FILE)->ReadAsString(content);
  Storage::ParseTwmGridJson(content, version, blocks, coverage);
  auto const owner =
      std::find_if(coverage.begin(), coverage.end(), [](auto const & entry) { return entry.second.size() == 1; });
  TEST(owner != coverage.end(), ());
  auto const & block = blocks[owner->second.front()];
  auto const file = MakeTerrainFile(block, block.m_version);

  tests_support::ScopedDir terrainDir(TERRAIN_DIR);
  tests_support::ScopedDir versionDir(terrainDir, strings::to_string(block.m_version));
  tests_support::ScopedFile blockFile(base::JoinPath(versionDir.GetRelativePath(), block.m_id + TERRAIN_FILE_EXT),
                                      "twm");
  Storage storage;
  size_t registrations = 0;
  std::vector<TerrainId> deleted;
  storage.SetTerrainCallbacks([&](terrain::TwmFile const &)
  {
    ++registrations;
    return true;
  }, [&](std::vector<TerrainId> const & ids) { deleted = ids; });

  // A deletion requested while the scan runs must be applied when it finishes.
  storage.DeleteTerrain(owner->first);
  storage.OnTerrainScanned({file});
  TEST_EQUAL(deleted, (std::vector<TerrainId>{block.m_id}), ());
  TEST_EQUAL(registrations, 1, ());
  TEST_EQUAL(storage.GetTerrainAttrs(owner->first).m_status, Storage::TerrainStatus::NotDownloaded, ());
  TEST(blockFile.Exists(), ());

  // A delayed scan cannot revive a file still held by a deregistering reader.
  storage.OnTerrainScanned({file});
  TEST_EQUAL(registrations, 1, ());
  TEST_EQUAL(storage.GetTerrainAttrs(owner->first).m_status, Storage::TerrainStatus::NotDownloaded, ());
  storage.OnTerrainFileDeregistered(file);
  TEST(!blockFile.Exists(), ());
  blockFile.Reset();

  // Nor can a scan snapshot revive the file after its deletion completes.
  storage.OnTerrainScanned({file});
  TEST_EQUAL(registrations, 1, ());
}

UNIT_TEST(Storage_TerrainAttrsAllNodes)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  Storage storage;
  ScanTerrain(storage);
  storage.SetTerrainCallbacks({}, {});

  // The downloader UI queries every row, including folders with no installed maps. Every
  // downloadable leaf must have coverage in data/twm_grid.json (the bundle guard: the
  // grid and data/borders drift independently), World/WorldCoasts must have none.
  size_t checked = 0;
  storage.ForEachInSubtree(storage.GetRootId(), [&](CountryId const & id, bool /* groupNode */)
  {
    auto const attrs = storage.GetTerrainAttrs(id);
    if (storage.IsLeaf(id) && id != WORLD_FILE_NAME && id != WORLD_COASTS_FILE_NAME)
      TEST_NOT_EQUAL(attrs.m_status, Storage::TerrainStatus::NotAvailable, (id));
    else
    {
      TEST_EQUAL(attrs.m_status, Storage::TerrainStatus::NotAvailable, (id));
      TEST_EQUAL(attrs.m_remainingSize, 0, (id));
    }
    ++checked;
  });
  TEST_GREATER(checked, 1000, (checked));

  TEST_EQUAL(storage.GetTerrainAttrs(WORLD_FILE_NAME).m_status, Storage::TerrainStatus::NotAvailable, ());
}

UNIT_TEST(Storage_TerrainDelete)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  Storage storage;
  storage.OnTerrainScanned({});

  std::vector<TerrainId> deleted;
  storage.SetTerrainCallbacks({}, [&](std::vector<TerrainId> const & ids) { deleted = ids; });
  storage.DeleteTerrain("Madagascar");
  TEST(deleted.empty(), ());  // Missing files need no deregistration request.

  int64_t version = 0;
  std::vector<Storage::TerrainBlock> blocks;
  std::map<CountryId, std::vector<uint32_t>> coverage;
  std::string content;
  GetPlatform().GetReader(TERRAIN_GRID_FILE)->ReadAsString(content);
  Storage::ParseTwmGridJson(content, version, blocks, coverage);
  std::set<uint32_t> installed(coverage.at("Madagascar").begin(), coverage.at("Madagascar").end());
  std::set<TerrainId> expected;
  storage.ForEachInSubtree("Norway", [&](CountryId const & id, bool groupNode)
  {
    if (!groupNode)
      for (auto const index : coverage.at(id))
      {
        installed.insert(index);
        expected.insert(blocks[index].m_id);
      }
  });
  tests_support::ScopedDir terrainDir(TERRAIN_DIR);
  std::list<tests_support::ScopedDir> versionDirs;
  std::list<tests_support::ScopedFile> blockFiles;
  std::set<int64_t> versions;
  std::vector<terrain::TwmFile> files;
  for (auto const index : installed)
  {
    auto const & block = blocks[index];
    if (versions.insert(block.m_version).second)
      versionDirs.emplace_back(terrainDir, strings::to_string(block.m_version));
    blockFiles.emplace_back(
        base::JoinPath(TERRAIN_DIR, strings::to_string(block.m_version), block.m_id + TERRAIN_FILE_EXT), "twm");
    files.push_back(MakeTerrainFile(block, block.m_version));
  }
  storage.OnTerrainScanned(std::move(files));

  storage.DeleteTerrain("Madagascar");
  TEST_EQUAL(deleted.size(), coverage.at("Madagascar").size(), ());
  for (auto const index : coverage.at("Madagascar"))
    TEST(std::find(deleted.begin(), deleted.end(), blocks[index].m_id) != deleted.end(), (index));

  // A group requests every installed block of its leaves exactly once.
  deleted.clear();
  storage.DeleteTerrain("Norway");
  TEST_EQUAL(deleted.size(), expected.size(), ());
  TEST_EQUAL((std::set<TerrainId>(deleted.begin(), deleted.end())), expected, ());
  deleted.clear();
  storage.DeleteTerrain("Norway");
  TEST(deleted.empty(), ());
}

UNIT_TEST(Storage_TerrainParseTwmGridJson)
{
  int64_t version = 42;
  std::vector<Storage::TerrainBlock> blocks;
  std::map<CountryId, std::vector<uint32_t>> coverage;

  // The per-block "v" falls back to the index version; the coverage resolves to the
  // deduplicated sorted indices.
  auto const indices = Storage::ParseTwmGridJson(R"({"v": 260729, "blocks": [
      {"id": "N45E008", "sx": 3, "sy": 2, "s": 10, "h": "aGFzaDE"},
      {"id": "N45E011", "sx": 2, "sy": 2, "s": 20, "h": "aGFzaDI", "v": 260315}],
      "mwms": {"A": ["N45E011", "N45E008", "N45E011"], "B": ["N45E008"]}})",
                                                 version, blocks, coverage);
  TEST_EQUAL(version, 260729, ());
  TEST_EQUAL(blocks.size(), 2, ());
  TEST_EQUAL(indices.at("N45E008"), 0, ());
  TEST_EQUAL(indices.at("N45E011"), 1, ());
  TEST_EQUAL(blocks[0].m_id, "N45E008", ());
  TEST(AlmostEqualAbs(blocks[0].m_rect, m2::RectD(mercator::FromLatLon(45, 8), mercator::FromLatLon(47, 11)),
                      kRectCompareEpsilon),
       ());
  TEST_EQUAL(blocks[0].m_size, 10, ());
  TEST_EQUAL(blocks[0].m_hash, "aGFzaDE", ());
  TEST_EQUAL(blocks[0].m_version, 260729, ());
  TEST_EQUAL(blocks[1].m_version, 260315, ());
  TEST_EQUAL(coverage.size(), 2, ());
  TEST_EQUAL(coverage["A"], (std::vector<uint32_t>{0, 1}), ());
  TEST_EQUAL(coverage["B"], (std::vector<uint32_t>{0}), ());

  // Any inconsistency throws and leaves the out params untouched: a truncated grid
  // must not half-configure the storage.
  for (char const * bad : {
           R"({"blocks": [{"id": "N45E008", "sx": 1, "sy": 1, "s": 1, "h": "aA"}], "mwms": {"A": ["N45E008"]}})",
           R"({"v": 1, "blocks": [{"id": "N45E008", "sx": 1, "sy": 1, "s": 1, "h": "aA"}]})",
           R"({"v": 1, "blocks": [{"id": "N45E008", "sx": 1, "sy": 1, "s": 1, "h": "aA"}], "mwms": {"A": []}})",
           R"({"v": 1, "blocks": [{"id": "N45E008", "sx": 1, "sy": 1, "s": 1, "h": "aA"}], "mwms": {"A": ["X"]}})",
           R"({"v": 1, "blocks": [{"id": "N45E008", "sx": 1, "sy": 1, "s": 1, "h": "aA"},
                                  {"id": "N45E008", "sx": 1, "sy": 1, "s": 1, "h": "aA"}],
               "mwms": {"A": ["N45E008"]}})",
           R"({"v": 1, "blocks": [{"id": "N45E008", "sx": 1, "sy": 1, "s": 1, "h": "aA", "v": -1}],
               "mwms": {"A": ["N45E008"]}})",
       })
  {
    bool thrown = false;
    try
    {
      Storage::ParseTwmGridJson(bad, version, blocks, coverage);
    }
    catch (RootException const &)
    {
      thrown = true;
    }
    TEST(thrown, (bad));
    TEST_EQUAL(version, 260729, (bad));
    TEST_EQUAL(blocks.size(), 2, (bad));
    TEST_EQUAL(coverage.size(), 2, (bad));
  }
}

UNIT_TEST(Storage_TerrainStatusPrecedence)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  // A region with some blocks on disk: Partly; a stale block whose area still renders
  // from an older file ranks OnDiskOutOfDate above Partly (a partial-world grid update
  // reads "update available", not "partly downloaded").
  int64_t version = 0;
  std::vector<Storage::TerrainBlock> blocks;
  std::map<CountryId, std::vector<uint32_t>> coverage;
  {
    std::string content;
    GetPlatform().GetReader(TERRAIN_GRID_FILE)->ReadAsString(content);
    Storage::ParseTwmGridJson(content, version, blocks, coverage);
  }

  // Any multi-block region works the same; pick a deterministic one.
  CountryId region;
  for (auto const & [id, indices] : coverage)
    if (indices.size() >= 2)
    {
      region = id;
      break;
    }
  TEST(!region.empty(), ());

  auto const & block = blocks[coverage[region].front()];
  tests_support::ScopedDir terrainDir(TERRAIN_DIR);
  tests_support::ScopedDir versionDir(terrainDir, strings::to_string(block.m_version));
  tests_support::ScopedFile blockFile(base::JoinPath(versionDir.GetRelativePath(), block.m_id + TERRAIN_FILE_EXT),
                                      "twm");

  Storage storage;
  auto files = ScanTerrain(storage);
  TEST_EQUAL(storage.GetTerrainAttrs(region).m_status, Storage::TerrainStatus::Partly, (region));

  auto const & missingBlock = blocks[coverage[region][1]];
  tests_support::ScopedDir olderDir(terrainDir, strings::to_string(missingBlock.m_version - 1));
  tests_support::ScopedFile olderFile(base::JoinPath(olderDir.GetRelativePath(), missingBlock.m_id + TERRAIN_FILE_EXT),
                                      "twm");
  files.push_back(MakeTerrainFile(missingBlock, missingBlock.m_version - 1));
  storage.OnTerrainScanned(std::move(files));
  TEST_EQUAL(storage.GetTerrainAttrs(region).m_status, Storage::TerrainStatus::OnDiskOutOfDate, (region));
}

UNIT_TEST(Storage_TerrainRefcountDelete)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;

  int64_t version = 0;
  std::vector<Storage::TerrainBlock> blocks;
  std::map<CountryId, std::vector<uint32_t>> coverage;
  {
    std::string content;
    GetPlatform().GetReader(TERRAIN_GRID_FILE)->ReadAsString(content);
    Storage::ParseTwmGridJson(content, version, blocks, coverage);
  }

  // Two regions sharing a block, the first one with a globally exclusive block too.
  std::map<uint32_t, std::vector<CountryId>> owners;
  for (auto const & [region, indices] : coverage)
    for (auto const index : indices)
      owners[index].push_back(region);

  CountryId regionA, regionB;
  uint32_t sharedBlock = 0, exclusiveBlock = 0;
  for (auto const & [index, regions] : owners)
  {
    if (regions.size() != 2)
      continue;
    for (auto const & region : regions)
    {
      auto const & indices = coverage[region];
      auto const exclusive =
          std::find_if(indices.begin(), indices.end(), [&owners](uint32_t i) { return owners[i].size() == 1; });
      if (exclusive == indices.end())
        continue;
      regionA = region;
      regionB = regions[0] == region ? regions[1] : regions[0];
      sharedBlock = index;
      exclusiveBlock = *exclusive;
      break;
    }
    if (!regionA.empty())
      break;
  }
  TEST(!regionA.empty() && !regionB.empty(), ());

  // All the blocks of both regions on disk (empty stub files are enough for the stats).
  std::set<uint32_t> created(coverage[regionA].begin(), coverage[regionA].end());
  created.insert(coverage[regionB].begin(), coverage[regionB].end());
  tests_support::ScopedDir terrainDir(TERRAIN_DIR);
  std::list<tests_support::ScopedDir> versionDirs;
  std::list<tests_support::ScopedFile> files;
  std::set<std::string> versionNames;
  for (auto const index : created)
    if (versionNames.insert(strings::to_string(blocks[index].m_version)).second)
      versionDirs.emplace_back(terrainDir, strings::to_string(blocks[index].m_version));
  for (auto const index : created)
    files.emplace_back(
        base::JoinPath(TERRAIN_DIR, strings::to_string(blocks[index].m_version), blocks[index].m_id + TERRAIN_FILE_EXT),
        "twm");

  // The terrain follows the maps: only regionB is downloaded, so its coverage is the
  // protection set of the ref-counted delete.
  Storage storage;
  tests_support::ScopedDir mapsDir(strings::to_string(storage.GetCurrentDataVersion()));
  tests_support::ScopedFile mapB(mapsDir, platform::CountryFile(regionB), MapFileType::Map);
  ResizeToRemote(mapB, storage, regionB);
  storage.RegisterAllLocalMaps();
  ScanTerrain(storage);
  std::vector<TerrainId> deleted;
  storage.SetTerrainCallbacks({}, [&](std::vector<TerrainId> const & ids) { deleted = ids; });
  TEST_EQUAL(storage.GetTerrainAttrs(regionA).m_status, Storage::TerrainStatus::OnDisk, (regionA));

  // Deleting regionA keeps the block shared with the downloaded regionB and drops the
  // globally exclusive one.
  std::set<uint32_t> wantedByB(coverage[regionB].begin(), coverage[regionB].end());
  size_t expectedA = 0;
  for (auto const index : coverage[regionA])
    if (wantedByB.count(index) == 0)
      ++expectedA;
  TEST_GREATER(expectedA, 0, ());
  deleted.clear();
  storage.DeleteTerrain(regionA);
  TEST_EQUAL(deleted.size(), expectedA, (regionA));
  auto const contains = [&deleted](TerrainId const & id)
  { return std::find(deleted.begin(), deleted.end(), id) != deleted.end(); };
  TEST(contains(blocks[exclusiveBlock].m_id), ());
  TEST(!contains(blocks[sharedBlock].m_id), ());

  // A deleted region does not protect itself: every block of regionB goes, the shared
  // one loses its last downloaded owner.
  deleted.clear();
  storage.DeleteTerrain(regionB);
  TEST_EQUAL(deleted.size(), coverage[regionB].size(), (regionB));
  TEST(contains(blocks[sharedBlock].m_id), ());
}

UNIT_TEST(Storage_TerrainOrphanReadySweep)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;

  // An interrupted download leaves the downloader artifacts behind; with no downloaded
  // map wanting the block, the startup restore sweeps them (see RestoreTerrain: the
  // resume runs after both the scan and the queue restore have landed).
  tests_support::ScopedDir terrainDir(TERRAIN_DIR);
  tests_support::ScopedDir versionDir(terrainDir, "260729");
  tests_support::ScopedFile ready(base::JoinPath(TERRAIN_DIR, "260729", "N45E008.twm.ready"), "partial");
  tests_support::ScopedFile resume(base::JoinPath(TERRAIN_DIR, "260729", "N45E008.twm.ready.resume"), "state");

  Storage storage;
  ScanTerrain(storage);
  storage.RestoreDownloadQueue();

  TEST(!ready.Exists(), ());
  TEST(!resume.Exists(), ());
  // The sweep already deleted them; a double delete in the dtor would log an error.
  ready.Reset();
  resume.Reset();
}

UNIT_TEST(Storage_TerrainArtifactResumeAndCancel)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;

  // The artifacts of an interrupted block under a downloaded map are the resume
  // record: the restore re-enqueues the region and keeps the artifacts (exercised in
  // the production iOS/macOS order - the queue restore lands first, the scan second);
  // a cancel deletes them, so the stopped download does not come back at a next start.
  int64_t version = 0;
  std::vector<Storage::TerrainBlock> blocks;
  std::map<CountryId, std::vector<uint32_t>> coverage;
  {
    std::string content;
    GetPlatform().GetReader(TERRAIN_GRID_FILE)->ReadAsString(content);
    Storage::ParseTwmGridJson(content, version, blocks, coverage);
  }
  auto const it =
      std::find_if(coverage.begin(), coverage.end(), [](auto const & entry) { return entry.second.size() == 1; });
  TEST(it != coverage.end(), ());
  CountryId const region = it->first;
  auto const & block = blocks[it->second.front()];

  tests_support::ScopedDir terrainDir(TERRAIN_DIR);
  tests_support::ScopedDir versionDir(terrainDir, strings::to_string(block.m_version));
  std::string const readyRel = base::JoinPath(TERRAIN_DIR, strings::to_string(block.m_version),
                                              block.m_id + TERRAIN_FILE_EXT READY_FILE_EXTENSION);
  tests_support::ScopedFile ready(readyRel + DOWNLOADING_FILE_EXTENSION, "partial");
  tests_support::ScopedFile resume(readyRel + RESUME_FILE_EXTENSION, "state");

  Storage storage;
  tests_support::ScopedDir mapsDir(strings::to_string(storage.GetCurrentDataVersion()));
  tests_support::ScopedFile map(mapsDir, platform::CountryFile(region), MapFileType::Map);
  ResizeToRemote(map, storage, region);
  storage.RegisterAllLocalMaps();
  TaskRunner runner;  // Never run: the resumed block must stay queued.
  storage.SetDownloaderForTesting(std::make_unique<FakeMapFilesDownloader>(runner));

  storage.RestoreDownloadQueue();
  ScanTerrain(storage);

  TEST_EQUAL(storage.GetTerrainAttrs(region).m_status, Storage::TerrainStatus::Downloading, (region));
  TEST(ready.Exists(), ());
  TEST(resume.Exists(), ());

  storage.CancelTerrain(region);
  TEST_EQUAL(storage.GetTerrainAttrs(region).m_status, Storage::TerrainStatus::NotDownloaded, (region));
  TEST(!ready.Exists(), ());
  TEST(!resume.Exists(), ());
  ready.Reset();
  resume.Reset();
}

UNIT_TEST(Storage_TerrainUpdateNetGrowth)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  tests_support::ScopedFile countries("terrain_growth.json", R"({"id":"Countries","v":260101,"g":[
    {"id":"Group","g":[{"id":"West","s":100},{"id":"East","s":200}]},
    {"id":"Other","g":[{"id":"West","s":100}]}]})");
  tests_support::ScopedDir mapsDir("260101");
  tests_support::ScopedFile west("260101/West.mwm", std::string(100, 'm'));
  tests_support::ScopedFile east("260101/East.mwm", std::string(200, 'm'));
  tests_support::ScopedDir terrainDir(TERRAIN_DIR);
  tests_support::ScopedDir versionDir(terrainDir, "1");
  tests_support::ScopedDirCleanup const currentVersionDir(
      base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR, "2"));
  std::string const splitGrid = R"({"v":2,"blocks":[
    {"id":"N45E008","sx":3,"sy":2,"s":30,"h":"aA"},
    {"id":"N45E011","sx":3,"sy":2,"s":30,"h":"aA"}],
    "mwms":{"West":["N45E008"],"East":["N45E011"]}})";
  std::string const mergedGrid = R"({"v":2,"blocks":[
    {"id":"N45E008","sx":6,"sy":2,"s":60,"h":"aA"}],
    "mwms":{"West":["N45E008"],"East":["N45E008"]}})";
  using OldBlock = std::pair<terrain::GridBlock, size_t>;
  auto const check = [&](std::string const & gridJson, std::vector<OldBlock> const & oldBlocks, CountryId const & node,
                         bool queueWest, uint64_t downloadSize, int64_t difference, uint64_t maxFileSize)
  {
    tests_support::ScopedFile grid(TERRAIN_GRID_FILE, gridJson);
    std::vector<std::unique_ptr<tests_support::ScopedFile>> oldFiles;
    std::vector<terrain::TwmFile> scanned;
    for (auto const & [block, size] : oldBlocks)
    {
      auto file = std::make_unique<tests_support::ScopedFile>(base::JoinPath(TERRAIN_DIR, "1", block.GetFileName()),
                                                              std::string(size, 't'));
      scanned.push_back(
          {base::FilenameWithoutExt(block.GetFileName()), 1, file->GetFullPath(), block.GetRectMercator()});
      oldFiles.push_back(std::move(file));
    }
    Storage storage("terrain_growth.json");
    storage.RegisterAllLocalMaps();
    storage.OnTerrainScanned(std::move(scanned));
    storage.SetDownloaderForTesting(std::make_unique<PendingTerrainDownloader>());
    if (queueWest)
      storage.DownloadTerrain("West");
    Storage::UpdateInfo info;
    TEST(storage.GetUpdateInfo(node, info), (node));
    TEST_EQUAL(info.m_totalDownloadSizeInBytes, downloadSize, (node, queueWest));
    TEST_EQUAL(info.m_sizeDifference, difference, (node, queueWest));
    TEST_EQUAL(info.m_maxFileSizeInBytes, maxFileSize, (node, queueWest));
    storage.CancelTerrain(storage.GetRootId());
  };
  // Actual local bytes, rather than overlap alone, determine growth and shrinkage.
  check(splitGrid, {{{8, 45, 3, 2}, 10}, {{11, 45, 3, 2}, 10}}, "Group", false, 60, 40, 30);
  check(splitGrid, {{{8, 45, 3, 2}, 50}, {{11, 45, 3, 2}, 50}}, "Group", false, 60, -40, 30);
  // One old file is subtracted once when split; a merged shared file is downloaded once.
  check(splitGrid, {{{8, 45, 6, 2}, 20}}, "Countries", false, 60, 40, 30);
  check(mergedGrid, {{{8, 45, 3, 2}, 10}, {{11, 45, 3, 2}, 10}}, "Countries", false, 60, 40, 60);
  // Another region's retained file and an already queued replacement give no credit.
  check(splitGrid, {{{8, 45, 3, 2}, 10}, {{11, 45, 3, 2}, 50}}, "West", false, 30, 20, 30);
  check(splitGrid, {{{8, 45, 3, 2}, 10}, {{11, 45, 3, 2}, 10}}, "Group", true, 30, 20, 30);
  check(mergedGrid, {{{8, 45, 6, 2}, 20}}, "Group", true, 0, 0, 0);
  check(splitGrid, {}, "Group", false, 60, 60, 30);
}

UNIT_TEST(Storage_TerrainUpdatesUseCombinedStatusAndActions)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  tests_support::ScopedFile countries("terrain_map_updates.json", R"({"id":"Countries","v":260101,
    "g":[{"id":"West","s":100},{"id":"East","s":200},{"id":"Missing","s":300}]})");
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, R"({"v":2,"blocks":[
    {"id":"N45E008","sx":3,"sy":2,"s":30,"h":"aA"},
    {"id":"N45E011","sx":3,"sy":2,"s":10,"h":"aA"}],
    "mwms":{"West":["N45E008"],"East":["N45E008"],"Missing":["N45E011"]}})");
  tests_support::ScopedDir mapsDir("260101");
  tests_support::ScopedDir terrainDir(TERRAIN_DIR);
  tests_support::ScopedDir versionDir(terrainDir, "1");
  tests_support::ScopedDir currentVersionDir(terrainDir, "2");
  tests_support::ScopedFile west("260101/West.mwm", std::string(100, 'm'));
  tests_support::ScopedFile east("260101/East.mwm", std::string(200, 'm'));
  tests_support::ScopedFile oldTerrain("terrain/1/N45E008.twm", "old terrain");
  for (bool const older : {false, true})
  {
    Storage storage("terrain_map_updates.json");
    storage.RegisterAllLocalMaps();
    if (older)
      storage.OnTerrainScanned(
          {{"N45E008", 1, oldTerrain.GetFullPath(), terrain::GridBlock{8, 45, 3, 2}.GetRectMercator()}});
    else
      storage.OnTerrainScanned({});
    TEST_EQUAL(storage.GetTerrainAttrs("West").m_status,
               older ? Storage::TerrainStatus::OnDiskOutOfDate : Storage::TerrainStatus::NotDownloaded, ());
    TEST_EQUAL(storage.GetTerrainDownloadSize({"West", "East", "West"}), 30, ());
    for (auto const & id : {CountryId("West"), CountryId("East"), storage.GetRootId()})
    {
      Storage::UpdateInfo info;
      TEST(storage.GetUpdateInfo(id, info), (id));
      TEST_EQUAL(info.m_numberOfMwmFilesToUpdate, storage.IsLeaf(id) ? 1 : 2, (id));
      TEST_EQUAL(info.m_totalDownloadSizeInBytes, 30, (id));
      NodeStatuses statuses;
      storage.GetNodeStatuses(id, statuses);
      TEST_EQUAL(statuses.m_status, NodeStatus::OnDiskOutOfDate, (id));
    }
    auto downloader = std::make_unique<PendingTerrainDownloader>();
    auto const & queue = downloader->GetQueue();
    storage.SetDownloaderForTesting(std::move(downloader));
    storage.UpdateNode(storage.GetRootId());
    storage.DownloadNode("West");
    TEST_EQUAL(queue.Count(), 1, ());
    NodeAttrs attrs;
    storage.GetNodeAttrs("West", attrs);
    TEST_EQUAL(attrs.m_status, NodeStatus::Downloading, ());
    TEST_EQUAL(attrs.m_downloadingProgress.m_bytesTotal, 30, ());
    storage.DownloadTerrain("West");
    TEST_EQUAL(queue.Count(), 1, ());
    TEST_EQUAL(storage.GetTerrainDownloadSize({"West", "East"}), 0, ());
    storage.CancelDownloadNode(storage.GetRootId());
    TEST(queue.IsEmpty(), ());
    storage.GetNodeAttrs("West", attrs);
    TEST_EQUAL(attrs.m_status, NodeStatus::OnDiskOutOfDate, ());
  }
}

// West and East share a block; Missing has terrain but no installed MWM in the folder tests.
char const kFolderCountries[] = R"({"id":"Countries","v":260101,"g":[{"id":"Group","g":[
  {"id":"West","s":100,"old":["OldWest"]},{"id":"East","s":200},{"id":"Missing","s":300}]}]})";
char const kFolderTerrainGrid[] = R"({"v":1,"blocks":[
  {"id":"N45E008","sx":3,"sy":2,"s":30,"h":"aA"},
  {"id":"N45E011","sx":3,"sy":2,"s":10,"h":"aA"},
  {"id":"N45E014","sx":3,"sy":2,"s":20,"h":"aA"},
  {"id":"N45E017","sx":3,"sy":2,"s":5,"h":"aA"}],
  "mwms":{"West":["N45E008","N45E011"],"East":["N45E008","N45E014"],"Missing":["N45E017"]}})";

UNIT_TEST(Storage_TerrainFolderWithMissingTilesIsPartly)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  tests_support::ScopedFile countries("terrain_folder.json", kFolderCountries);
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, R"({"v":1,"blocks":[
    {"id":"N45E008","sx":3,"sy":2,"s":30,"h":"aA"},
    {"id":"N45E011","sx":3,"sy":2,"s":10,"h":"aA"},
    {"id":"N45E014","sx":3,"sy":2,"s":20,"h":"aA"}],
    "mwms":{"West":["N45E008","N45E011"],"East":["N45E014"]}})");
  tests_support::ScopedDir mapsDir("260101");
  tests_support::ScopedFile west("260101/West.mwm", std::string(100, 'm'));
  tests_support::ScopedFile east("260101/East.mwm", std::string(200, 'm'));
  tests_support::ScopedDir terrainDir(TERRAIN_DIR);
  tests_support::ScopedDir versionDir(terrainDir, "1");
  tests_support::ScopedFile shared("terrain/1/N45E008.twm", std::string(30, 't'));
  tests_support::ScopedFile exclusive("terrain/1/N45E011.twm", std::string(10, 't'));
  Storage storage("terrain_folder.json");
  storage.RegisterAllLocalMaps();
  storage.OnTerrainScanned(
      {{"N45E008", 1, shared.GetFullPath(), terrain::GridBlock{8, 45, 3, 2}.GetRectMercator()},
       {"N45E011", 1, exclusive.GetFullPath(), terrain::GridBlock{11, 45, 3, 2}.GetRectMercator()}});
  TEST_EQUAL(storage.GetTerrainAttrs("West").m_status, Storage::TerrainStatus::OnDisk, ());
  TEST_EQUAL(storage.GetTerrainAttrs("East").m_status, Storage::TerrainStatus::NotDownloaded, ());
  for (auto const & id : {CountryId("Group"), storage.GetRootId()})
  {
    auto const attrs = storage.GetTerrainAttrs(id);
    TEST_EQUAL(attrs.m_status, Storage::TerrainStatus::Partly, (id));
    TEST_EQUAL(attrs.m_localSize, 40, (id));
    TEST_EQUAL(attrs.m_remainingSize, 20, (id));
    TEST_EQUAL(storage.GetTerrainDownloadSize({id}), 20, (id));
  }
  tests_support::ScopedFile eastTerrain("terrain/1/N45E014.twm", std::string(20, 't'));
  storage.OnTerrainScanned(
      {{"N45E014", 1, eastTerrain.GetFullPath(), terrain::GridBlock{14, 45, 3, 2}.GetRectMercator()}});
  storage.DownloadTerrain("East");
  TEST_EQUAL(storage.GetTerrainAttrs("Group").m_status, Storage::TerrainStatus::OnDisk, ());
  CountriesSet notified;
  std::vector<Storage::TerrainStatus> parentStatuses;
  auto const slot = storage.Subscribe([&](CountryId const & id)
  {
    notified.insert(id);
    if (id == "Group")
      parentStatuses.push_back(storage.GetTerrainAttrs(id).m_status);
  }, [](CountryId const &, storage::Progress const &) {});
  storage.DeleteTerrain("East");
  TEST(!eastTerrain.Exists(), ());
  eastTerrain.Reset();
  TEST_EQUAL(storage.GetTerrainAttrs("East").m_status, Storage::TerrainStatus::NotDownloaded, ());
  TEST_EQUAL(storage.GetTerrainAttrs("Group").m_status, Storage::TerrainStatus::Partly, ());
  TEST(notified.contains("East") && notified.contains("Group"), (notified));
  TEST(!parentStatuses.empty(), ());
  for (auto const status : parentStatuses)
    TEST_EQUAL(status, Storage::TerrainStatus::Partly, ());
  storage.Unsubscribe(slot);
  // Folder terrain deletion removes its last owners' files without deleting their MWMs.
  storage.DeleteTerrain("Group");
  TEST(!shared.Exists() && !exclusive.Exists(), ());
  shared.Reset();
  exclusive.Reset();
  TEST(west.Exists() && east.Exists(), ());
  TEST_EQUAL(storage.GetTerrainAttrs("Group").m_status, Storage::TerrainStatus::NotDownloaded, ());
}

UNIT_TEST(Storage_IdenticalTerrainCoverageDeletesAfterLastMap)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  tests_support::ScopedFile countries("terrain_identical.json", R"({"id":"Countries","v":260101,
    "g":[{"id":"Group","g":[{"id":"West","s":100},{"id":"East","s":200}]}]})");
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, R"({"v":1,"blocks":[
    {"id":"N45E008","sx":3,"sy":2,"s":30,"h":"aA"},
    {"id":"N45E011","sx":3,"sy":2,"s":10,"h":"aA"}],
    "mwms":{"West":["N45E008","N45E011"],"East":["N45E008","N45E011"]}})");
  tests_support::ScopedDir mapsDir("260101");
  for (bool const otherRequested : {false, true})
  {
    tests_support::ScopedFile west("260101/West.mwm", std::string(100, 'm'));
    tests_support::ScopedFile east("260101/East.mwm", std::string(200, 'm'));
    tests_support::ScopedDir terrainDir(TERRAIN_DIR);
    tests_support::ScopedDir versionDir(terrainDir, "1");
    tests_support::ScopedFile first("terrain/1/N45E008.twm", std::string(30, 't'));
    tests_support::ScopedFile second("terrain/1/N45E011.twm", std::string(10, 't'));
    Storage storage("terrain_identical.json");
    storage.Init([](CountryId const &, LocalFilePtr const) {},
                 [](CountryId const &, LocalFilePtr const) { return false; });
    storage.RegisterAllLocalMaps();
    storage.OnTerrainScanned({});  // Establish separate-download intent before importing test files.
    storage.OnTerrainScanned(
        {{"N45E008", 1, first.GetFullPath(), terrain::GridBlock{8, 45, 3, 2}.GetRectMercator()},
         {"N45E011", 1, second.GetFullPath(), terrain::GridBlock{11, 45, 3, 2}.GetRectMercator()}});
    storage.DownloadTerrain("West");
    if (otherRequested)
      storage.DownloadTerrain("East");
    TEST_EQUAL(storage.GetTerrainAttrs("Group").m_status, Storage::TerrainStatus::OnDisk, (otherRequested));
    storage.DeleteNode("West");
    west.Reset();
    // East uses the shared coverage even if West was the only explicit terrain request.
    TEST(first.Exists() && second.Exists(), (otherRequested));
    TEST_EQUAL(storage.GetTerrainAttrs("East").m_status, Storage::TerrainStatus::OnDisk, (otherRequested));
    std::string saved;
    TEST(settings::Get("TerrainRequestedRegions", saved), ());
    TEST_EQUAL(saved, otherRequested ? "East" : "", (otherRequested));
    storage.DeleteNode("East");
    east.Reset();
    TEST(!first.Exists() && !second.Exists(), (otherRequested));
    first.Reset();
    second.Reset();
    TEST_EQUAL(storage.GetTerrainAttrs("East").m_status, Storage::TerrainStatus::NotDownloaded, (otherRequested));
    TEST(!west.Exists() && !east.Exists(), (otherRequested));
  }
}

UNIT_TEST(Storage_TerrainFolderIgnoresUninstalledMaps)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  tests_support::ScopedFile countries("terrain_folder.json", kFolderCountries);
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, kFolderTerrainGrid);
  tests_support::ScopedDir mapsDir("260100");
  tests_support::ScopedFile west("260100/West.mwm", "old");
  tests_support::ScopedDirCleanup const terrainDir(base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR));
  tests_support::ScopedDir versionDir("terrain/1");
  tests_support::ScopedFile shared("terrain/1/N45E008.twm", std::string(30, 't'));
  tests_support::ScopedFile exclusive("terrain/1/N45E011.twm", std::string(10, 't'));
  Storage storage("terrain_folder.json");
  storage.RegisterAllLocalMaps();
  storage.OnTerrainScanned(
      {{"N45E008", 1, shared.GetFullPath(), terrain::GridBlock{8, 45, 3, 2}.GetRectMercator()},
       {"N45E011", 1, exclusive.GetFullPath(), terrain::GridBlock{11, 45, 3, 2}.GetRectMercator()}});
  auto downloader = std::make_unique<PendingTerrainDownloader>();
  auto & pending = *downloader;
  storage.SetDownloaderForTesting(std::move(downloader));
  auto const checkFolder = [&]
  {
    for (auto const & id : {CountryId("Group"), storage.GetRootId()})
    {
      auto const attrs = storage.GetTerrainAttrs(id);
      TEST_EQUAL(attrs.m_status, Storage::TerrainStatus::OnDisk, (id));
      TEST_EQUAL(attrs.m_localSize, 40, (id));
      TEST_EQUAL(attrs.m_remainingSize, 0, (id));
      TEST_EQUAL(attrs.m_downloadingProgress.m_bytesDownloaded, 40, (id));
      TEST_EQUAL(attrs.m_downloadingProgress.m_bytesTotal, 40, (id));
      TEST_EQUAL(storage.GetTerrainDownloadSize({id}), 0, (id));
    }
  };
  checkFolder();
  // A valid old MWM still belongs to the folder while its replacement is queued.
  storage.UpdateNode("West");
  checkFolder();
  storage.CancelDownloadNode("West");
  storage.DownloadNode("East");
  checkFolder();
  storage.DownloadTerrain("East");
  storage.CancelDownloadNode("East");
  TEST(pending.GetQueue().IsEmpty(), ());
  storage.DownloadTerrain("East");
  TEST_EQUAL(pending.GetQueue().Count(), 1, ());
  pending.Start();
  pending.Progress(10);
  TEST_EQUAL(storage.GetTerrainAttrs("East").m_status, Storage::TerrainStatus::Downloading, ());
  checkFolder();
  // Notification progress includes explicit terrain-only leaves independently of folder rows.
  for (auto const & id : {CountryId("Group"), storage.GetRootId()})
  {
    auto const progress = storage.GetDownloadingProgress(id);
    TEST_EQUAL(progress.m_twmDownloaded, 40, (id));
    TEST_EQUAL(progress.m_twmTotal, 50, (id));
    TEST_EQUAL(storage.GetDownloadingProgress(id).m_mwmTotal, 0, (id));
  }
  pending.Fail(downloader::DownloadStatus::FileNotFound);
  TEST_EQUAL(storage.GetTerrainAttrs("East").m_status, Storage::TerrainStatus::Failed, ());
  checkFolder();
  storage.DownloadTerrain("Group");
  TEST(pending.GetQueue().IsEmpty(), ());
  // Deleting a folder still clears explicit requests throughout its full subtree.
  storage.DeleteTerrain("Group");
  TEST(!shared.Exists(), ());
  TEST(!exclusive.Exists(), ());
  shared.Reset();
  exclusive.Reset();
  TEST_EQUAL(storage.GetTerrainAttrs("East").m_status, Storage::TerrainStatus::NotDownloaded, ());
  TEST_EQUAL(storage.GetTerrainAttrs("Group").m_remainingSize, 40, ());
}

UNIT_TEST(Storage_TerrainFolderDownloadsInstalledMapsOnly)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  tests_support::ScopedFile countries("terrain_folder.json", kFolderCountries);
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, kFolderTerrainGrid);
  tests_support::ScopedDir oldMapsDir("260100");
  tests_support::ScopedDir mapsDir("260101");
  tests_support::ScopedFile west("260100/OldWest.mwm", "old");
  tests_support::ScopedFile east("260101/East.mwm", std::string(200, 'm'));
  tests_support::ScopedDirCleanup const terrainDir(base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR));
  Storage storage("terrain_folder.json");
  storage.OnTerrainScanned({});
  auto downloader = std::make_unique<PendingTerrainDownloader>();
  auto & pending = *downloader;
  storage.SetDownloaderForTesting(std::move(downloader));
  for (auto const & id : {CountryId("Group"), storage.GetRootId()})
  {
    auto const attrs = storage.GetTerrainAttrs(id);
    TEST_EQUAL(attrs.m_status, Storage::TerrainStatus::NotAvailable, (id));
    TEST_EQUAL(attrs.m_localSize, 0, (id));
    TEST_EQUAL(attrs.m_remainingSize, 0, (id));
    TEST_EQUAL(attrs.m_downloadingProgress.m_bytesTotal, 0, (id));
    TEST_EQUAL(storage.GetTerrainDownloadSize({id}), 0, (id));
    storage.DownloadTerrain(id);
    TEST(pending.GetQueue().IsEmpty(), (id));
  }
  storage.RegisterAllLocalMaps();
  NodeStatuses statuses;
  storage.GetNodeStatuses("West", statuses);
  TEST_EQUAL(statuses.m_status, NodeStatus::OnDiskOutOfDate, ());
  CountriesVec downloaded, available;
  storage.GetChildrenInGroups(storage.GetRootId(), downloaded, available, false);
  TEST_EQUAL(downloaded, CountriesVec{"Group"}, ());
  TEST(available.empty(), ());
  storage.GetChildrenInGroups("Group", downloaded, available, false);
  CountriesVec const expectedDownloaded = {"West", "East"};
  TEST_EQUAL(downloaded, expectedDownloaded, ());
  TEST_EQUAL(available, CountriesVec{"Missing"}, ());
  for (auto const & id : {CountryId("Group"), storage.GetRootId()})
  {
    auto const attrs = storage.GetTerrainAttrs(id);
    TEST_EQUAL(attrs.m_status, Storage::TerrainStatus::NotDownloaded, (id));
    TEST_EQUAL(attrs.m_remainingSize, 60, (id));
    TEST_EQUAL(storage.GetTerrainDownloadSize({id, "West", "East"}), 60, (id));
  }
  // The shared block is scheduled once; the uninstalled sibling's exclusive block is omitted.
  storage.DownloadTerrain("Group");
  TEST_EQUAL(pending.GetQueue().Count(), 3, ());
  TEST(!pending.GetQueue().Contains("N45E017"), ());
  TEST_EQUAL(storage.GetTerrainDownloadSize({"Group"}), 0, ());
  storage.DownloadTerrain(storage.GetRootId());
  TEST_EQUAL(pending.GetQueue().Count(), 3, ());
  std::string saved;
  TEST(settings::Get("TerrainRequestedRegions", saved), ());
  TEST_EQUAL(saved, "East;West", ());
  storage.CancelTerrain("Group");
  TEST(pending.GetQueue().IsEmpty(), ());
  // A leaf remains actionable without its MWM, and ancestor cancellation still reaches it.
  TEST_EQUAL(storage.GetTerrainAttrs("Missing").m_remainingSize, 5, ());
  TEST_EQUAL(storage.GetTerrainDownloadSize({"Missing"}), 5, ());
  storage.DownloadTerrain("Missing");
  TEST_EQUAL(pending.GetQueue().Count(), 1, ());
  TEST_EQUAL(storage.GetTerrainAttrs("Missing").m_status, Storage::TerrainStatus::Downloading, ());
  TEST_EQUAL(storage.GetTerrainAttrs("Group").m_status, Storage::TerrainStatus::NotDownloaded, ());
  TEST_EQUAL(storage.GetTerrainAttrs("Group").m_remainingSize, 60, ());
  storage.CancelTerrain(storage.GetRootId());
  TEST(pending.GetQueue().IsEmpty(), ());
}

UNIT_TEST(Storage_TerrainFolderRequestRestoresWithoutMaps)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  tests_support::ScopedFile countries("terrain_folder.json", kFolderCountries);
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, kFolderTerrainGrid);
  tests_support::ScopedDirCleanup const terrainDir(base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR));
  settings::Set("TerrainDownloadQueue", std::string{"Group"});
  Storage storage("terrain_folder.json");
  auto downloader = std::make_unique<PendingTerrainDownloader>();
  auto & pending = *downloader;
  storage.SetDownloaderForTesting(std::move(downloader));
  storage.RestoreDownloadQueue();
  storage.OnTerrainScanned({});
  TEST_EQUAL(pending.GetQueue().Count(), 4, ());
  for (auto const & id : {CountryId("West"), CountryId("East"), CountryId("Missing")})
    TEST_EQUAL(storage.GetTerrainAttrs(id).m_status, Storage::TerrainStatus::Downloading, (id));
  for (auto const & id : {CountryId("Group"), storage.GetRootId()})
  {
    TEST_EQUAL(storage.GetTerrainAttrs(id).m_status, Storage::TerrainStatus::NotAvailable, (id));
    TEST_EQUAL(storage.GetTerrainAttrs(id).m_remainingSize, 0, (id));
    TEST_EQUAL(storage.GetDownloadingProgress(id).m_twmTotal, 65, (id));
    TEST_EQUAL(storage.GetDownloadingProgress(id).m_mwmTotal, 0, (id));
  }
  std::string saved;
  TEST(settings::Get("TerrainDownloadQueue", saved), ());
  TEST_EQUAL(saved, "East;Missing;West", ());
  storage.CancelTerrain("Group");
  TEST(pending.GetQueue().IsEmpty(), ());
}

UNIT_TEST(Storage_TerrainSplitSizes)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  tests_support::ScopedFile countries("terrain_sizes.json", R"({"id":"Countries","v":260101,
    "g":[{"id":"Group","g":[{"id":"West","s":100},{"id":"East","s":200},{"id":"MapOnly","s":300}]}]})");
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, R"({"v":1,"blocks":[
    {"id":"N45E008","sx":3,"sy":2,"s":30,"h":"aA"},
    {"id":"N45E011","sx":3,"sy":2,"s":10,"h":"aA"}],
    "mwms":{"West":["N45E008","N45E011"],"East":["N45E008"]}})");
  tests_support::ScopedDir oldMapsDir("260100");
  tests_support::ScopedDir mapsDir("260101");
  tests_support::ScopedDir terrainDir(TERRAIN_DIR);
  tests_support::ScopedDir versionDir(terrainDir, "1");
  tests_support::ScopedFile west(base::JoinPath(oldMapsDir.GetRelativePath(), "West.mwm"), "old");
  tests_support::ScopedFile east(base::JoinPath(mapsDir.GetRelativePath(), "East.mwm"), std::string(200, 'm'));
  tests_support::ScopedFile terrainFile(base::JoinPath(versionDir.GetRelativePath(), "N45E008.twm"), "terrain");
  Storage storage("terrain_sizes.json");
  storage.OnTerrainScanned({});

  NodeAttrs attrs;
  storage.GetNodeAttrs("West", attrs);
  TEST_EQUAL(attrs.m_mwmSize, 140, ());
  TEST_EQUAL(storage.GetTerrainAttrs("West").m_remainingSize, 40, ());
  TEST_EQUAL(storage.GetTerrainAttrs("West").m_localSize, 0, ());
  storage.GetNodeAttrs("Group", attrs);
  TEST_EQUAL(attrs.m_mwmSize, 640, ());
  TEST_EQUAL(storage.GetTerrainAttrs("Group").m_remainingSize, 0, ());  // No MWMs have been registered yet.

  storage.RegisterAllLocalMaps();
  storage.OnTerrainScanned(
      {{"N45E008", 1, terrainFile.GetFullPath(), terrain::GridBlock{8, 45, 3, 2}.GetRectMercator()}});
  storage.GetNodeAttrs("Group", attrs);
  TEST_EQUAL(attrs.m_localMwmSize, 210, ());
  TEST_EQUAL(storage.GetDownloadSize({"Group"}), 410, ());
  TEST_EQUAL(storage.GetTerrainAttrs("Group").m_localSize, 7, ());
  TEST_EQUAL(attrs.m_downloadSize, 110, ());
  storage.GetNodeAttrs(storage.GetRootId(), attrs);
  TEST_EQUAL(attrs.m_localMwmSize, 210, ());
  TEST_EQUAL(attrs.m_downloadSize, 110, ());
  storage.GetNodeAttrs("West", attrs);
  TEST_EQUAL(attrs.m_localMwmSize, 10, ());
  TEST_EQUAL(attrs.m_downloadSize, 110, ());
  storage.GetNodeAttrs("East", attrs);
  TEST_EQUAL(attrs.m_localMwmSize, 207, ());
  TEST_EQUAL(attrs.m_downloadSize, 0, ());
  TEST_EQUAL(storage.GetTerrainAttrs("Group").m_remainingSize, 10, ());

  Storage::UpdateInfo info;
  TEST(storage.GetUpdateInfo("Group", info), ());
  TEST_EQUAL(info.m_totalDownloadSizeInBytes, 110, ());
  TEST_EQUAL(storage.GetTerrainDownloadSize({"Group"}), 10, ());
  TEST(storage.GetUpdateInfo("East", info), ());
  TEST_EQUAL(info.m_totalDownloadSizeInBytes, 0, ());

  TaskRunner runner;  // Leave the new terrain block queued.
  storage.SetDownloaderForTesting(std::make_unique<FakeMapFilesDownloader>(runner));
  storage.DownloadTerrain("West");
  storage.GetNodeAttrs("West", attrs);
  TEST_EQUAL(attrs.m_downloadSize, 40, ());
  TEST_EQUAL(storage.GetTerrainAttrs("West").m_downloadingProgress.m_bytesTotal, 40, ());
  TEST_EQUAL(storage.GetTerrainAttrs("West").m_localSize, 7, ());
  // A map without coverage has no terrain attributes.
  storage.GetNodeAttrs("MapOnly", attrs);
  TEST_EQUAL(attrs.m_mwmSize, 300, ());
  TEST_EQUAL(attrs.m_downloadSize, 300, ());
  TEST_EQUAL(storage.GetTerrainAttrs("MapOnly").m_localSize, 0, ());
  TEST_EQUAL(storage.GetTerrainAttrs("MapOnly").m_remainingSize, 0, ());
  storage.CancelTerrain("West");
  storage.SetDiffInfoForTesting({{"West", {10, 260100}}});
  for (auto const & id : {CountryId("West"), CountryId("Group"), storage.GetRootId()})
  {
    storage.GetNodeAttrs(id, attrs);
    TEST_EQUAL(attrs.m_downloadSize, 20, (id));  // The MWM diff plus missing terrain.
  }
  TEST_EQUAL(storage.GetDownloadSize({"Group"}), 410, ());  // Download All requests full MWMs, not diffs.
}

UNIT_TEST(Storage_TerrainLeafAndFolderSizes)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  tests_support::ScopedFile countries("terrain_display_sizes.json", R"({"id":"Countries","v":260101,
    "g":[{"id":"Group","g":[{"id":"West","s":100},{"id":"East","s":200},{"id":"Missing","s":300}]}]})");
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, R"({"v":1,"blocks":[
    {"id":"N45E008","sx":3,"sy":2,"s":30,"h":"aA"},
    {"id":"N45E011","sx":3,"sy":2,"s":10,"h":"aA"}],
    "mwms":{"West":["N45E008"],"East":["N45E008"],"Missing":["N45E011"]}})");
  tests_support::ScopedDir mapsDir("260101");
  tests_support::ScopedDir terrainDir(TERRAIN_DIR);
  tests_support::ScopedDir versionDir(terrainDir, "1");
  tests_support::ScopedFile west("260101/West.mwm", std::string(100, 'm'));
  tests_support::ScopedFile east("260101/East.mwm", std::string(200, 'm'));
  tests_support::ScopedFile shared("terrain/1/N45E008.twm", std::string(30, 't'));
  tests_support::ScopedFile cached("terrain/1/N45E011.twm", std::string(10, 't'));
  for (bool const complete : {false, true})
  {
    std::unique_ptr<tests_support::ScopedFile> missing;
    if (complete)
      missing = std::make_unique<tests_support::ScopedFile>("260101/Missing.mwm", std::string(300, 'm'));
    Storage storage("terrain_display_sizes.json");
    storage.RegisterAllLocalMaps();
    storage.OnTerrainScanned({
        {"N45E008", 1, shared.GetFullPath(), terrain::GridBlock{8, 45, 3, 2}.GetRectMercator()},
        {"N45E011", 1, cached.GetFullPath(), terrain::GridBlock{11, 45, 3, 2}.GetRectMercator()},
    });
    NodeAttrs attrs;
    for (auto const & id : {CountryId("Group"), storage.GetRootId()})
    {
      storage.GetNodeAttrs(id, attrs);
      TEST_EQUAL(attrs.m_status, complete ? NodeStatus::OnDisk : NodeStatus::Partly, (id, complete));
      TEST_EQUAL(attrs.m_localMwmSize, complete ? 640 : 330, (id, complete));
      TEST_EQUAL(attrs.m_mwmSize, 640, (id, complete));
      TEST_EQUAL(attrs.m_downloadSize, complete ? 0 : 300, (id, complete));
    }
    storage.GetNodeAttrs("West", attrs);
    TEST_EQUAL(attrs.m_status, NodeStatus::OnDisk, ());
    TEST_EQUAL(attrs.m_localMwmSize, 130, ());
    TEST_EQUAL(attrs.m_downloadSize, 0, ());
    storage.GetNodeAttrs("East", attrs);
    TEST_EQUAL(attrs.m_localMwmSize, 230, ());
    storage.GetNodeAttrs("Missing", attrs);
    TEST_EQUAL(attrs.m_localMwmSize, complete ? 310 : 10, ());
    TEST_EQUAL(attrs.m_downloadSize, complete ? 0 : 300, ());
  }
}

UNIT_CLASS_TEST(StorageTerrainDownloadTest, SharedTerrainProgressBelongsToRequestOwners)
{
  uint64_t constexpr kSize = 2 * 1024 * 1024;
  tests_support::ScopedFile countries("terrain_shared_notifications.json", R"({"id":"Countries","v":260101,"g":[
    {"id":"WestCountry","g":[{"id":"West","s":2097152}]},
    {"id":"EastCountry","g":[{"id":"East","s":2097152}]},
    {"id":"NorthCountry","g":[{"id":"North","s":2097152}]}]})");
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, R"({"v":1,
    "blocks":[{"id":"N45E008","sx":3,"sy":2,"s":2097152,"h":"aA"}],
    "mwms":{"West":["N45E008"],"East":["N45E008"],"North":["N45E008"]}})");
  tests_support::ScopedDirCleanup const mapsDir(base::JoinPath(GetPlatform().WritableDir(), "260101"));
  tests_support::ScopedDirCleanup const terrainDir(base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR));
  tests_support::ScopedFile west("260101/West.mwm", std::string(kSize, 'm'));
  tests_support::ScopedFile east("260101/East.mwm", std::string(kSize, 'm'));
  tests_support::ScopedFile north("260101/North.mwm", std::string(kSize, 'm'));
  Storage storage("terrain_shared_notifications.json");
  storage.SetEnabledIntegrityValidationForTesting(false);
  storage.RegisterAllLocalMaps();
  storage.OnTerrainScanned({});
  storage.SetTerrainCallbacks([](auto const &)
  {
    testing::StopEventLoop();
    return true;
  }, {});
  auto downloader = std::make_unique<PendingTerrainDownloader>();
  auto & pending = *downloader;
  storage.SetDownloaderForTesting(std::move(downloader));
  CountriesSet statuses, progresses;
  auto const slot = storage.Subscribe([&](CountryId const & id) { statuses.insert(id); },
                                      [&](CountryId const & id, storage::Progress const &) { progresses.insert(id); });
  auto const checkInactive = [&](CountryId const & id)
  {
    auto const attrs = storage.GetTerrainAttrs(id);
    TEST_EQUAL(attrs.m_status, Storage::TerrainStatus::NotDownloaded, (id));
    TEST_EQUAL(attrs.m_remainingSize, kSize, (id));
    TEST_EQUAL(attrs.m_downloadingProgress.m_bytesDownloaded, 0, (id));
    TEST_EQUAL(attrs.m_downloadingProgress.m_bytesTotal, 0, (id));
    TEST_EQUAL(storage.GetDownloadingProgress(id).m_mwmTotal, 0, (id));
    TEST_EQUAL(storage.GetDownloadingProgress(id).m_twmTotal, 0, (id));
  };
  auto const checkActive = [&](CountryId const & id, uint64_t downloaded)
  {
    auto const attrs = storage.GetTerrainAttrs(id);
    TEST_EQUAL(attrs.m_status, Storage::TerrainStatus::Downloading, (id));
    TEST_EQUAL(attrs.m_downloadingProgress.m_bytesDownloaded, static_cast<int64_t>(downloaded), (id));
    TEST_EQUAL(attrs.m_downloadingProgress.m_bytesTotal, static_cast<int64_t>(kSize), (id));
    TEST_EQUAL(storage.GetDownloadingProgress(id).m_mwmDownloaded, 0, (id));
    TEST_EQUAL(storage.GetDownloadingProgress(id).m_mwmTotal, 0, (id));
    auto const terrainProgress = storage.GetDownloadingProgress(id);
    TEST_EQUAL(terrainProgress.m_twmDownloaded, static_cast<int64_t>(downloaded), (id));
    TEST_EQUAL(terrainProgress.m_twmTotal, static_cast<int64_t>(kSize), (id));
  };
  auto const checkOnly = [&](CountriesSet const & notifications, CountriesSet const & expected)
  { TEST_EQUAL(notifications, expected, ()); };
  CountriesSet const eastStatusHierarchy = {"East", "EastCountry"};
  CountriesSet const westStatusHierarchy = {"West", "WestCountry"};
  CountriesSet const allStatusHierarchy = {"West", "WestCountry", "East", "EastCountry", "North", "NorthCountry"};
  CountriesSet const eastHierarchy = {"East", "EastCountry", storage.GetRootId()};
  CountriesSet const westHierarchy = {"West", "WestCountry", storage.GetRootId()};
  CountriesSet const allHierarchy = {"West",         "WestCountry",      "East", "EastCountry", "North",
                                     "NorthCountry", storage.GetRootId()};
  storage.DownloadTerrain("East");
  checkOnly(statuses, eastStatusHierarchy);
  statuses.clear();
  pending.Start();
  checkOnly(statuses, eastStatusHierarchy);
  statuses.clear();
  pending.Progress(kSize / 2);
  checkOnly(progresses, eastHierarchy);
  for (auto const & id : eastHierarchy)
    checkActive(id, kSize / 2);
  for (auto const * id : {"West", "WestCountry", "North", "NorthCountry"})
    checkInactive(id);

  // Polling or cancelling an untouched neighbor does not acquire the live request.
  storage.CancelTerrain("West");
  TEST(statuses.empty(), (statuses));
  TEST_EQUAL(pending.GetQueue().Count(), 1, ());
  storage.DownloadTerrain("West");
  TEST_EQUAL(pending.m_requests, 1, ());
  checkActive("West", kSize / 2);
  checkActive("WestCountry", kSize / 2);
  checkInactive("North");
  statuses.clear();
  storage.CancelTerrain("East");
  TEST_EQUAL(pending.GetQueue().Count(), 1, ());
  checkInactive("East");
  checkInactive("EastCountry");
  checkActive("West", kSize / 2);
  TEST(!statuses.contains("North") && !statuses.contains("NorthCountry"), (statuses));
  statuses.clear();
  progresses.clear();
  pending.Progress(kSize);
  checkOnly(progresses, westHierarchy);
  pending.Fail(downloader::DownloadStatus::FileNotFound);
  checkOnly(statuses, westStatusHierarchy);
  TEST_EQUAL(storage.GetTerrainAttrs("West").m_status, Storage::TerrainStatus::Failed, ());
  checkInactive("East");
  checkInactive("North");

  storage.DownloadTerrain("West");
  TEST_EQUAL(pending.m_requests, 2, ());
  checkActive("West", 0);
  storage.CancelTerrain("West");
  storage.DownloadTerrain("East");
  pending.Start();
  statuses.clear();
  progresses.clear();
  pending.Complete(TerrainHeaderContent());
  testing::RunEventLoop();
  // Only published bytes refresh coverage neighbors, without granting request ownership.
  checkOnly(statuses, allStatusHierarchy);
  TEST(progresses.empty(), (progresses));
  for (auto const & id : allHierarchy)
  {
    TEST_EQUAL(storage.GetTerrainAttrs(id).m_status, Storage::TerrainStatus::OnDisk, (id));
    TEST_EQUAL(storage.GetDownloadingProgress(id).m_mwmTotal, 0, (id));
  }
  storage.DeleteTerrain("North");
  TEST_EQUAL(storage.GetTerrainAttrs("East").m_status, Storage::TerrainStatus::OnDisk, ());
  storage.DeleteTerrain("East");
  TEST_EQUAL(storage.GetTerrainAttrs("North").m_status, Storage::TerrainStatus::OnDisk, ());
  storage.DeleteTerrain(storage.GetRootId());
  TEST_EQUAL(storage.GetTerrainAttrs("North").m_status, Storage::TerrainStatus::NotDownloaded, ());
  storage.Unsubscribe(slot);
}

UNIT_TEST(Storage_TerrainFolderProgressExcludesUntappedRegions)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  tests_support::ScopedFile countries("terrain_folder.json", kFolderCountries);
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, kFolderTerrainGrid);
  tests_support::ScopedDir mapsDir("260101");
  tests_support::ScopedFile west("260101/West.mwm", std::string(100, 'm'));
  tests_support::ScopedFile east("260101/East.mwm", std::string(200, 'm'));
  tests_support::ScopedDirCleanup const terrainDir(base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR));
  Storage storage("terrain_folder.json");
  storage.RegisterAllLocalMaps();
  storage.OnTerrainScanned({});
  auto downloader = std::make_unique<PendingTerrainDownloader>();
  auto & pending = *downloader;
  storage.SetDownloaderForTesting(std::move(downloader));
  storage.DownloadTerrain("West");
  pending.Start();
  pending.Progress(15);
  for (auto const & id : {CountryId("Group"), storage.GetRootId()})
  {
    auto const attrs = storage.GetTerrainAttrs(id);
    TEST_EQUAL(attrs.m_status, Storage::TerrainStatus::Downloading, (id));
    TEST_EQUAL(attrs.m_remainingSize, 20, (id));  // The unrequested East block is still missing.
    TEST_EQUAL(attrs.m_downloadingProgress.m_bytesDownloaded, 15, (id));
    TEST_EQUAL(attrs.m_downloadingProgress.m_bytesTotal, 40, (id));  // Only the requested West MWM is active.
    TEST_EQUAL(storage.GetDownloadingProgress(id).m_twmTotal, 40, (id));
    TEST_EQUAL(storage.GetDownloadingProgress(id).m_mwmTotal, 0, (id));
  }
  TEST_EQUAL(storage.GetTerrainAttrs("East").m_status, Storage::TerrainStatus::NotDownloaded, ());
  TEST_EQUAL(storage.GetTerrainAttrs("East").m_downloadingProgress.m_bytesDownloaded, 0, ());
  TEST_EQUAL(storage.GetTerrainAttrs("East").m_remainingSize, 50, ());
  TEST_EQUAL(storage.GetDownloadingProgress("East").m_mwmTotal, 0, ());
  storage.CancelTerrain("West");
}

UNIT_TEST(Storage_TerrainCanCancelFromProgressCallback)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  tests_support::ScopedFile countries("terrain_folder.json", kFolderCountries);
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, R"({"v":1,
    "blocks":[{"id":"N45E008","sx":3,"sy":2,"s":2097152,"h":"aA"}],
    "mwms":{"West":["N45E008"],"East":["N45E008"]}})");
  tests_support::ScopedDirCleanup const terrainDir(base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR));
  Storage storage("terrain_folder.json");
  storage.OnTerrainScanned({});
  auto downloader = std::make_unique<PendingTerrainDownloader>();
  auto & pending = *downloader;
  storage.SetDownloaderForTesting(std::move(downloader));
  storage.DownloadTerrain("West");
  storage.DownloadTerrain("East");
  CountriesSet notified;
  bool cancelled = false;
  auto const slot = storage.Subscribe([](CountryId const &) {}, [&](CountryId const & id, storage::Progress const &)
  {
    notified.insert(id);
    if (!cancelled)
    {
      cancelled = true;
      storage.CancelTerrain(storage.GetRootId());
    }
  });
  pending.Start();
  if (!cancelled)
    pending.Progress(1024 * 1024);
  TEST(cancelled, ());
  TEST(pending.GetQueue().IsEmpty(), ());
  TEST(!storage.IsDownloadInProgress(), ());
  CountriesSet const expected = {"West", "East", "Group", storage.GetRootId()};
  TEST_EQUAL(notified, expected, ());
  storage.Unsubscribe(slot);
}

UNIT_TEST(Storage_PassiveTerrainHasNoLiveProgress)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  tests_support::ScopedFile countries("terrain_folder.json", kFolderCountries);
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, kFolderTerrainGrid);
  tests_support::ScopedDir mapsDir("260101");
  tests_support::ScopedFile west("260101/West.mwm", std::string(100, 'm'));
  tests_support::ScopedFile east("260101/East.mwm", std::string(200, 'm'));
  for (bool const older : {false, true})
  {
    tests_support::ScopedDirCleanup const terrainDir(base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR));
    tests_support::ScopedDir versionDir(older ? "terrain/0" : "terrain/1");
    tests_support::ScopedFile existing(base::JoinPath(versionDir.GetRelativePath(), "N45E014.twm"),
                                       std::string(20, 't'));
    Storage storage("terrain_folder.json");
    storage.RegisterAllLocalMaps();
    storage.OnTerrainScanned(
        {{"N45E014", older ? 0 : 1, existing.GetFullPath(), terrain::GridBlock{14, 45, 3, 2}.GetRectMercator()}});
    auto downloader = std::make_unique<PendingTerrainDownloader>();
    auto & pending = *downloader;
    storage.SetDownloaderForTesting(std::move(downloader));
    storage.DownloadTerrain("West");
    pending.Start();
    pending.Progress(15);
    // Independent map progress callbacks can poll these attrs; an idle fraction must not start a terrain spinner.
    auto const attrs = storage.GetTerrainAttrs("East");
    TEST_EQUAL(attrs.m_status, older ? Storage::TerrainStatus::OnDiskOutOfDate : Storage::TerrainStatus::Partly,
               (older));
    TEST_EQUAL(attrs.m_localSize, 20, (older));
    TEST_EQUAL(attrs.m_remainingSize, older ? 50 : 30, (older));
    TEST_EQUAL(attrs.m_downloadingProgress.m_bytesDownloaded, 0, (older));
    TEST_EQUAL(attrs.m_downloadingProgress.m_bytesTotal, 0, (older));
    storage.CancelTerrain("West");
  }
}

UNIT_TEST(Storage_SharedTerrainRestoreKeepsOnlyRequestedProgress)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  tests_support::ScopedFile countries("terrain_shared_restore.json", R"({"id":"Countries","v":260101,
    "g":[{"id":"West","s":100},{"id":"East","s":200}]})");
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, R"({"v":1,
    "blocks":[{"id":"N45E008","sx":3,"sy":2,"s":2097152,"h":"aA"}],
    "mwms":{"West":["N45E008"],"East":["N45E008"]}})");
  tests_support::ScopedDirCleanup const terrainDir(base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR));
  {
    Storage storage("terrain_shared_restore.json");
    storage.OnTerrainScanned({});
    storage.SetDownloaderForTesting(std::make_unique<PendingTerrainDownloader>());
    storage.DownloadTerrain("West");
  }
  Storage storage("terrain_shared_restore.json");
  auto downloader = std::make_unique<PendingTerrainDownloader>();
  auto & pending = *downloader;
  storage.SetDownloaderForTesting(std::move(downloader));
  storage.RestoreDownloadQueue();
  storage.OnTerrainScanned({});
  TEST_EQUAL(pending.GetQueue().Count(), 1, ());
  pending.Start();
  pending.Progress(1024 * 1024);
  TEST_EQUAL(storage.GetTerrainAttrs("West").m_status, Storage::TerrainStatus::Downloading, ());
  TEST_EQUAL(storage.GetTerrainAttrs("West").m_downloadingProgress.m_bytesDownloaded, 1024 * 1024, ());
  TEST_EQUAL(storage.GetTerrainAttrs("East").m_status, Storage::TerrainStatus::NotDownloaded, ());
  TEST_EQUAL(storage.GetTerrainAttrs("East").m_downloadingProgress.m_bytesDownloaded, 0, ());
  CountriesVec downloaded, available;
  storage.GetChildrenInGroups(storage.GetRootId(), downloaded, available, false);
  TEST(downloaded.empty(), ());
  CountriesVec const expectedAvailable = {"West", "East"};
  TEST_EQUAL(available, expectedAvailable, ());
  // Terrain progress includes standalone requests even when folders select no MWMs.
  TEST_EQUAL(storage.GetTerrainAttrs(storage.GetRootId()).m_status, Storage::TerrainStatus::NotAvailable, ());
  TEST_EQUAL(storage.GetDownloadingProgress(storage.GetRootId()).m_twmDownloaded, 1024 * 1024, ());
  TEST_EQUAL(storage.GetDownloadingProgress(storage.GetRootId()).m_mwmTotal, 0, ());
  TEST_EQUAL(storage.GetDownloadingProgress("East").m_mwmTotal, 0, ());
  storage.CancelTerrain(storage.GetRootId());
  TEST(pending.GetQueue().IsEmpty(), ());
}

UNIT_TEST(Storage_DownloadAndCancelCoverMapsAndTerrain)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  tests_support::ScopedFile countries("independent_downloads.json", R"({"id":"Countries","v":260101,
    "g":[{"id":"Group","g":[{"id":"West","s":100},{"id":"East","s":200}]}]})");
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, R"({"v":1,"blocks":[
    {"id":"N45E008","sx":3,"sy":2,"s":30,"h":"aA"},
    {"id":"N45E011","sx":3,"sy":2,"s":10,"h":"aA"}],
    "mwms":{"West":["N45E008","N45E011"],"East":["N45E008"]}})");
  Storage storage("independent_downloads.json");
  storage.OnTerrainScanned({});
  auto downloader = std::make_unique<PendingTerrainDownloader>();
  auto & pending = *downloader;
  auto const & queue = pending.GetQueue();
  storage.SetDownloaderForTesting(std::move(downloader));
  TEST_EQUAL(storage.GetDownloadSize({"Group", "West"}), 340, ());
  TEST_EQUAL(storage.GetTerrainDownloadSize({"Group", "West", "East"}), 40, ());
  storage.DownloadNode("Group");
  TEST_EQUAL(queue.Count(), 4, ());
  TEST_EQUAL(storage.GetTerrainAttrs("West").m_status, Storage::TerrainStatus::Downloading, ());
  storage.DownloadTerrain("West");
  storage.DownloadTerrain("East");
  TEST_EQUAL(queue.Count(), 4, ());
  storage.DownloadTerrain("West");
  TEST_EQUAL(queue.Count(), 4, ());
  storage.CancelDownloadNode("Group");
  TEST(queue.IsEmpty(), ());
  TEST_EQUAL(storage.GetTerrainAttrs("West").m_status, Storage::TerrainStatus::NotDownloaded, ());
  TEST_EQUAL(storage.GetDownloadSize({"Group"}), 340, ());
  TEST_EQUAL(storage.GetTerrainDownloadSize({"Group"}), 0, ());
  storage.DownloadNode("West");
  storage.DownloadTerrain("East");
  storage.CancelTerrain("West");
  // East still owns the shared terrain block, and cancelling terrain leaves MWM queued.
  TEST(queue.Contains("West"), ());
  TEST(queue.Contains("N45E008"), ());
  TEST(!queue.Contains("N45E011"), ());
  storage.CancelTerrain("East");
  TEST_EQUAL(queue.Count(), 1, ());
  storage.CancelDownloadNode("West");
  TEST(queue.IsEmpty(), ());
}

UNIT_TEST(Storage_TerrainProgressCallbacks)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  uint64_t constexpr kMegabyte = 1024 * 1024;
  tests_support::ScopedFile countries("terrain_progress_countries.json", R"({"id": "Countries", "v": 260101,
    "g": [{"id": "Group", "g": [{"id": "West", "s": 2097152}, {"id": "East", "s": 2097152}]},
          {"id": "Installed", "s": 1048576}]})");

  for (bool const withTerrain : {false, true})
  {
    tests_support::ScopedFile grid(TERRAIN_GRID_FILE, withTerrain ? R"({"v": 1,
      "blocks": [{"id": "N45E008", "sx": 3, "sy": 2, "s": 4194304, "h": "aA"},
                 {"id": "N45E011", "sx": 3, "sy": 2, "s": 4194304, "h": "aA"}],
      "mwms": {"West": ["N45E008", "N45E011"], "East": ["N45E008"]}})"
                                                                  : R"({"v": 1, "blocks": [], "mwms": {}})");
    Storage storage("terrain_progress_countries.json");
    storage.Init([](CountryId const &, LocalFilePtr const) {},
                 [](CountryId const &, LocalFilePtr const) { return false; });
    storage.SetEnabledIntegrityValidationForTesting(false);
    storage.OnTerrainScanned({});
    tests_support::ScopedDirCleanup const mapsDir(base::JoinPath(GetPlatform().WritableDir(), "260101"));
    tests_support::ScopedDirCleanup const terrainDir(base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR));
    // An unrelated installed map must not inflate the active root download progress.
    tests_support::ScopedFile installedMap("260101/Installed.mwm", std::string(kMegabyte, 'm'));
    storage.RegisterAllLocalMaps();
    auto downloader = std::make_unique<PendingTerrainDownloader>();
    auto & pending = *downloader;
    storage.SetDownloaderForTesting(std::move(downloader));

    std::map<CountryId, storage::Progress> received;
    std::map<CountryId, size_t> notifications;
    size_t statusChanges = 0;
    auto const onProgress = [&](CountryId const & id, storage::Progress const & progress)
    {
      NodeAttrs attrs;
      storage.GetNodeAttrs(id, attrs);
      TEST_EQUAL(progress.m_mwmDownloaded, attrs.m_downloadingProgress.m_bytesDownloaded, (id, withTerrain));
      TEST_EQUAL(progress.m_mwmTotal, attrs.m_downloadingProgress.m_bytesTotal, (id, withTerrain));
      received[id] = progress;
      ++notifications[id];
    };
    auto const slot = storage.Subscribe([&](CountryId const &) { ++statusChanges; }, onProgress);
    auto const checkProgress = [&](CountryId const & id, uint64_t downloaded, uint64_t total)
    {
      auto const it = received.find(id);
      TEST(it != received.end(), (id, withTerrain));
      TEST_EQUAL(it->second.m_mwmDownloaded, static_cast<int64_t>(downloaded), (id, withTerrain));
      TEST_EQUAL(it->second.m_mwmTotal, static_cast<int64_t>(total), (id, withTerrain));
    };

    storage.DownloadCountry("West", MapFileType::Map);
    storage.DownloadCountry("East", MapFileType::Map);
    auto const westTotal = 2 * kMegabyte;
    auto const groupTotal = 4 * kMegabyte;
    pending.Start();
    pending.Progress(kMegabyte);
    checkProgress("West", kMegabyte, westTotal);
    checkProgress("Group", kMegabyte, groupTotal);
    checkProgress(storage.GetRootId(), kMegabyte, groupTotal);

    pending.Progress(2 * kMegabyte);
    checkProgress("West", 2 * kMegabyte, westTotal);
    pending.Complete(std::string(2 * kMegabyte, '\0'));
    NodeAttrs attrs;
    storage.GetNodeAttrs("West", attrs);
    TEST_EQUAL(attrs.m_downloadingProgress.m_bytesDownloaded, static_cast<int64_t>(2 * kMegabyte), (withTerrain));
    TEST_EQUAL(attrs.m_downloadingProgress.m_bytesTotal, static_cast<int64_t>(westTotal), (withTerrain));

    pending.Start();
    received.clear();
    notifications.clear();
    statusChanges = 0;
    pending.Progress(kMegabyte);
    checkProgress("East", kMegabyte, 2 * kMegabyte);
    checkProgress("Group", 3 * kMegabyte, groupTotal);
    checkProgress(storage.GetRootId(), 3 * kMegabyte, groupTotal);
    TEST_EQUAL(statusChanges, 0, (withTerrain));
    for (auto const & [id, count] : notifications)
      TEST_EQUAL(count, 1, (id, withTerrain));

    auto const overall = storage.GetOverallProgress({"West", "East", "West"});
    TEST_EQUAL(overall.m_bytesDownloaded, static_cast<int64_t>(3 * kMegabyte), (withTerrain));
    TEST_EQUAL(overall.m_bytesTotal, static_cast<int64_t>(groupTotal), (withTerrain));

    // Progress remains current while no UI is subscribed.
    storage.Unsubscribe(slot);
    pending.Progress(2 * kMegabyte);
    storage.GetNodeAttrs("East", attrs);
    TEST_EQUAL(attrs.m_downloadingProgress.m_bytesDownloaded, static_cast<int64_t>(2 * kMegabyte), (withTerrain));
    storage.CancelDownloadNode("Group");
  }
}

UNIT_CLASS_TEST(StorageTerrainDownloadTest, TerrainProgressUpdatesTheCombinedNodeProgress)
{
  uint64_t constexpr kMapSize = 2 * 1024 * 1024;
  // Cross the terrain notification byte threshold using a valid container payload.
  auto const firstContent = TerrainHeaderContent({8, 45, 3, 2}, 2 * 1024 * 1024);
  auto const secondContent = TerrainHeaderContent({11, 45, 3, 2});
  tests_support::ScopedFile countries("terrain_progress_batch.json", R"({"id":"Countries","v":260101,
    "g":[{"id":"Group","g":[{"id":"West","s":2097152},{"id":"East","s":2097152}]}]})");
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, R"({"v":1,"blocks":[
    {"id":"N45E008","sx":3,"sy":2,"s":)" + strings::to_string(firstContent.size()) +
                                                        R"(,"h":"aA"},
    {"id":"N45E011","sx":3,"sy":2,"s":)" + strings::to_string(secondContent.size()) +
                                                        R"(,"h":"aA"}],
    "mwms":{"West":["N45E008","N45E011"],"East":["N45E008"]}})");
  tests_support::ScopedDirCleanup const mapsDir(base::JoinPath(GetPlatform().WritableDir(), "260101"));
  tests_support::ScopedDirCleanup const terrainDir(base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR));
  tests_support::ScopedFile west("260101/West.mwm", std::string(kMapSize, 'm'));
  tests_support::ScopedFile east("260101/East.mwm", std::string(kMapSize, 'm'));
  Storage storage("terrain_progress_batch.json");
  storage.SetEnabledIntegrityValidationForTesting(false);
  storage.RegisterAllLocalMaps();
  storage.OnTerrainScanned({});
  storage.SetTerrainCallbacks([](auto const &)
  {
    testing::StopEventLoop();
    return true;
  }, {});
  auto downloader = std::make_unique<PendingTerrainDownloader>();
  auto & pending = *downloader;
  storage.SetDownloaderForTesting(std::move(downloader));
  std::map<CountryId, storage::Progress> mapProgress;
  auto const slot = storage.Subscribe([](auto const &) {},
                                      [&](CountryId const & id, storage::Progress const & p) { mapProgress[id] = p; });
  storage.DownloadTerrain("West");
  auto const total = firstContent.size() + secondContent.size();
  auto const check = [&](uint64_t downloaded)
  {
    // Both regions share the first block; watch each map and terrain file only once.
    auto const overall = storage.GetOverallProgress({"West", "East", "West"});
    if (storage.IsDownloadInProgress())
    {
      TEST_EQUAL(overall.m_bytesDownloaded, static_cast<int64_t>(2 * kMapSize + downloaded), ());
      TEST_EQUAL(overall.m_bytesTotal, static_cast<int64_t>(2 * kMapSize + total), ());
    }
    else
      TEST_EQUAL(overall.m_bytesDownloaded, overall.m_bytesTotal, ());
    for (auto const & id : {CountryId("West"), CountryId("Group"), storage.GetRootId()})
    {
      auto const terrain = storage.GetTerrainAttrs(id);
      TEST_EQUAL(terrain.m_downloadingProgress.m_bytesDownloaded, static_cast<int64_t>(downloaded), (id));
      TEST_EQUAL(terrain.m_downloadingProgress.m_bytesTotal, static_cast<int64_t>(total), (id));
      NodeAttrs attrs;
      storage.GetNodeAttrs(id, attrs);
      if (storage.IsDownloadInProgress())
      {
        TEST_EQUAL(attrs.m_status, NodeStatus::Downloading, (id));
        TEST_EQUAL(attrs.m_downloadingProgress.m_bytesDownloaded, static_cast<int64_t>(downloaded), (id));
        TEST_EQUAL(attrs.m_downloadingProgress.m_bytesTotal, static_cast<int64_t>(total), (id));
      }
      else
      {
        TEST_EQUAL(attrs.m_status, NodeStatus::OnDisk, (id));
        TEST_EQUAL(attrs.m_downloadingProgress.m_bytesDownloaded, attrs.m_downloadingProgress.m_bytesTotal, (id));
      }
    }
  };
  check(0);
  pending.Start();
  pending.Progress(firstContent.size() / 2);
  check(firstContent.size() / 2);
  for (auto const & id : {CountryId("West"), CountryId("Group"), storage.GetRootId()})
  {
    TEST(mapProgress.contains(id), (id));
    TEST_EQUAL(mapProgress.at(id).m_mwmDownloaded, 0, (id));
    TEST_EQUAL(mapProgress.at(id).m_mwmTotal, 0, (id));
    TEST_EQUAL(mapProgress.at(id).m_twmDownloaded, static_cast<int64_t>(firstContent.size() / 2), (id));
    TEST_EQUAL(mapProgress.at(id).m_twmTotal, static_cast<int64_t>(total), (id));
  }
  pending.Complete(firstContent);
  testing::RunEventLoop();
  check(firstContent.size());
  TEST_EQUAL(storage.GetTerrainAttrs("West").m_status, Storage::TerrainStatus::Downloading, ());
  pending.Start();
  pending.Complete(secondContent);
  testing::RunEventLoop();
  check(total);
  TEST(!storage.IsDownloadInProgress(), ());
  storage.Unsubscribe(slot);
}

UNIT_TEST(Storage_TerrainRetry)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;

  int64_t version = 0;
  std::vector<Storage::TerrainBlock> blocks;
  std::map<CountryId, std::vector<uint32_t>> coverage;
  {
    std::string content;
    GetPlatform().GetReader(TERRAIN_GRID_FILE)->ReadAsString(content);
    Storage::ParseTwmGridJson(content, version, blocks, coverage);
  }
  // A single-block region: one forced download failure fails its whole coverage.
  auto const owner =
      std::find_if(coverage.begin(), coverage.end(), [](auto const & entry) { return entry.second.size() == 1; });
  TEST(owner != coverage.end(), ());

  Storage const mapInfo;
  tests_support::ScopedDir mapsDir(strings::to_string(mapInfo.GetCurrentDataVersion()));
  tests_support::ScopedFile map(mapsDir, platform::CountryFile(owner->first), MapFileType::Map);
  ResizeToRemote(map, mapInfo, owner->first);

  // Only the transport failures may arm the auto-retry (cf. the failed maps): a 404 or
  // a hash mismatch would re-download a big block to the same end. Either way the
  // terrain status reports the failure; the MWM remains current.
  auto const run = [&](CountryId const & retryId, downloader::DownloadStatus status, size_t expectedRetryCalls)
  {
    TaskRunner runner;
    RecordingDownloadingPolicy policy;
    Storage storage;
    storage.SetDownloaderForTesting(std::make_unique<FakeMapFilesDownloader>(runner, std::vector{status}));
    storage.SetDownloadingPolicy(&policy);
    storage.RegisterAllLocalMaps();
    storage.OnTerrainScanned({});
    storage.SetTerrainCallbacks({}, {});
    storage.DownloadTerrain(owner->first);
    runner.Run();

    NodeAttrs attrs;
    storage.GetNodeAttrs(owner->first, attrs);
    TEST_EQUAL(attrs.m_status, NodeStatus::Error, (status));
    TEST_EQUAL(
        attrs.m_error,
        status == downloader::DownloadStatus::Failed ? NodeErrorCode::NoInetConnection : NodeErrorCode::UnknownError,
        (status));
    TEST(storage.HasLatestVersion(owner->first), (status));
    TEST_EQUAL(storage.GetTerrainAttrs(owner->first).m_status, Storage::TerrainStatus::Failed, (status));
    TEST(!storage.CheckFailedCountries({owner->first}), (status));
    TEST_EQUAL(policy.m_calls, expectedRetryCalls, (status));
    if (expectedRetryCalls > 0)
      TEST(policy.m_regions.count(owner->first) > 0, (status));

    // Explicit retry must requeue terrain even when its failure cannot auto-retry.
    storage.RetryDownloadNode(retryId);
    storage.GetNodeAttrs(owner->first, attrs);
    TEST_EQUAL(attrs.m_status, NodeStatus::Downloading, (retryId, status));
    TEST_EQUAL(attrs.m_error, NodeErrorCode::NoError, (retryId, status));
    TEST(!storage.CheckFailedCountries({owner->first}), (retryId, status));
    TEST_EQUAL(storage.GetTerrainAttrs(owner->first).m_status, Storage::TerrainStatus::Downloading, (retryId, status));
    storage.CancelTerrain(retryId);
  };

  for (auto const & retryId : {owner->first, mapInfo.GetRootId()})
  {
    run(retryId, downloader::DownloadStatus::FileNotFound, 0);
    run(retryId, downloader::DownloadStatus::Failed, 1);
  }
}

UNIT_TEST(Storage_TerrainOwnershipSurvivesRestartWithoutMaps)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  tests_support::ScopedFile countries("terrain_ownership.json", R"({"id":"Countries","v":260101,
    "g":[{"id":"Group","g":[{"id":"West","s":100},{"id":"East","s":200}]},
      {"id":"Other","s":300}]})");
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, R"({"v":1,"blocks":[
    {"id":"N45E008","sx":3,"sy":2,"s":30,"h":"aA"},
    {"id":"N45E011","sx":3,"sy":2,"s":10,"h":"aA"}],
    "mwms":{"West":["N45E008","N45E011"],"East":["N45E008"],"Other":["N45E008"]}})");
  tests_support::ScopedDir terrainDir(TERRAIN_DIR);
  tests_support::ScopedDir versionDir(terrainDir, "1");
  tests_support::ScopedFile shared("terrain/1/N45E008.twm", std::string(30, 't'));
  tests_support::ScopedFile exclusive("terrain/1/N45E011.twm", std::string(10, 't'));
  std::vector<terrain::TwmFile> const files = {
      {"N45E008", 1, shared.GetFullPath(), terrain::GridBlock{8, 45, 3, 2}.GetRectMercator()},
      {"N45E011", 1, exclusive.GetFullPath(), terrain::GridBlock{11, 45, 3, 2}.GetRectMercator()}};
  {
    Storage storage("terrain_ownership.json");
    storage.OnTerrainScanned(std::vector(files));
    storage.DownloadTerrain("West");  // Current files still establish explicit ownership.
    storage.DownloadTerrain("East");
    storage.Clear();  // Deregistering maps during a storage move retains installed terrain intent.
    std::string saved;
    TEST(settings::Get("TerrainRequestedRegions", saved), ());
    TEST_EQUAL(saved, "East;West", ());
  }
  Storage storage("terrain_ownership.json");
  storage.OnTerrainScanned(std::vector(files));
  CountriesVec downloaded, available;
  storage.GetChildrenInGroups(storage.GetRootId(), downloaded, available, false);
  TEST(downloaded.empty(), ());
  // Shared and explicitly requested terrain alone do not put a region in My Maps.
  CountriesVec const expectedAvailable = {"Group", "Other"};
  TEST_EQUAL(available, expectedAvailable, ());
  TEST_EQUAL(storage.GetDownloadedFilesCount(), 0, ());
  TEST_EQUAL(storage.GetTerrainAttrs("Group").m_status, Storage::TerrainStatus::NotAvailable, ());
  TEST_EQUAL(storage.GetTerrainAttrs("Group").m_remainingSize, 0, ());
  storage.DeleteNode("West");
  TEST(shared.Exists(), ());
  TEST(!exclusive.Exists(), ());
  exclusive.Reset();
  storage.DeleteNode("East");
  TEST(!shared.Exists(), ());
  shared.Reset();
  storage.GetChildrenInGroups(storage.GetRootId(), downloaded, available, false);
  TEST(downloaded.empty(), ());
}

UNIT_CLASS_TEST(StorageTerrainDownloadTest, CancelRestartTerrainValidation)
{
  auto const content = TerrainHeaderContent();
  tests_support::ScopedDirCleanup const terrainDir(base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR));
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, TerrainGridJson(content));
  Storage storage(COUNTRIES_FILE);
  storage.OnTerrainScanned({});
  auto downloader = std::make_unique<PendingTerrainDownloader>();
  auto & pending = *downloader;
  storage.SetDownloaderForTesting(std::move(downloader));
  size_t registrations = 0;
  storage.SetTerrainCallbacks([&](terrain::TwmFile const &)
  {
    ++registrations;
    return true;
  }, {});
  storage.DownloadTerrain("Madagascar");
  pending.Complete(content);

  // Leave the first validated attempt waiting for GUI publication.
  std::promise<void> published;
  GetPlatform().RunTask(Platform::Thread::File, [&] { published.set_value(); });
  published.get_future().wait();
  storage.CancelTerrain("Madagascar");
  storage.DownloadTerrain("Madagascar");
  TEST_EQUAL(pending.m_requests, 2, ());
  GetPlatform().RunTask(Platform::Thread::File,
                        [] { GetPlatform().RunTask(Platform::Thread::Gui, [] { testing::StopEventLoop(); }); });
  testing::RunEventLoop();

  auto const prematureStatus = storage.GetTerrainAttrs("Madagascar").m_status;
  auto const prematureRegistrations = registrations;
  // Deliver the second actual request, then drain its validator and GUI publication.
  pending.Complete(content);
  GetPlatform().RunTask(Platform::Thread::File,
                        [] { GetPlatform().RunTask(Platform::Thread::Gui, [] { testing::StopEventLoop(); }); });
  testing::RunEventLoop();

  std::string const finalPath = base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR, "1", "N45E008.twm");
  TEST_EQUAL(prematureStatus, Storage::TerrainStatus::Downloading, ());
  TEST_EQUAL(prematureRegistrations, 0, ());
  TEST(Platform::IsFileExistsByFullPath(finalPath), ());
  TEST_EQUAL(storage.GetTerrainAttrs("Madagascar").m_status, Storage::TerrainStatus::OnDisk, ());
}

UNIT_CLASS_TEST(StorageTerrainDownloadTest, CancelRestartTerrainBeforeValidation)
{
  auto const content = TerrainHeaderContent();
  tests_support::ScopedDirCleanup const terrainDir(base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR));
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, TerrainGridJson(content));
  Storage storage(COUNTRIES_FILE);
  storage.OnTerrainScanned({});
  auto downloader = std::make_unique<PendingTerrainDownloader>();
  auto & pending = *downloader;
  storage.SetDownloaderForTesting(std::move(downloader));
  size_t registrations = 0;
  storage.SetTerrainCallbacks([&](terrain::TwmFile const &)
  {
    ++registrations;
    return true;
  }, {});
  std::promise<void> release;
  auto released = release.get_future().share();
  GetPlatform().RunTask(Platform::Thread::File, [released] { released.wait(); });
  storage.DownloadTerrain("Madagascar");
  pending.Complete(content);
  storage.CancelTerrain("Madagascar");
  storage.DownloadTerrain("Madagascar");
  if (!pending.GetQueue().IsEmpty())
    pending.Complete(content);
  // Both attempts use the same ready path, but the first validator must retain
  // only its own bytes and leave publication to the restarted attempt.
  release.set_value();
  GetPlatform().RunTask(Platform::Thread::File,
                        [] { GetPlatform().RunTask(Platform::Thread::Gui, [] { testing::StopEventLoop(); }); });
  testing::RunEventLoop();
  std::string const finalPath = base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR, "1", "N45E008.twm");
  TEST_EQUAL(registrations, 1, ());
  TEST(Platform::IsFileExistsByFullPath(finalPath), ());
  TEST_EQUAL(storage.GetTerrainAttrs("Madagascar").m_status, Storage::TerrainStatus::OnDisk, ());
}
UNIT_CLASS_TEST(StorageTerrainDownloadTest, DeletePreservesValidatingOwnersTerrain)
{
  std::string const content(2 * 1024 * 1024, 'm');
  coding::Blake3 hasher;
  hasher.Update(content.data(), content.size());
  auto const hash = hasher.FinalizeToBase64(coding::Blake3::kMwmHashSizeInBytes);
  tests_support::ScopedFile countries("review_countries.json", R"({"id":"Countries","v":260101,"g":[
    {"id":"West","s":2097152},{"id":"East","s":2097152,"h":")" + hash +
                                                                   R"("}]})");
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE,
                                 R"({"v":1,"blocks":[{"id":"N45E008","sx":3,"sy":2,"s":100,"h":"aA"}],
    "mwms":{"West":["N45E008"],"East":["N45E008"]}})");
  tests_support::ScopedDirCleanup const mapsDir(base::JoinPath(GetPlatform().WritableDir(), "260101"));
  tests_support::ScopedDirCleanup const terrainRoot(base::JoinPath(GetPlatform().WritableDir(), "terrain"));
  tests_support::ScopedDirCleanup const terrainDir(base::JoinPath(GetPlatform().WritableDir(), "terrain/1"));
  tests_support::ScopedFile west("260101/West.mwm", content);
  tests_support::ScopedFile terrain("terrain/1/N45E008.twm", "terrain");
  Storage storage("review_countries.json");
  storage.Init([](CountryId const &, LocalFilePtr const) {},
               [](CountryId const &, LocalFilePtr const) { return false; });
  storage.RegisterAllLocalMaps();
  storage.OnTerrainScanned({{"N45E008", 1, terrain.GetFullPath(), terrain::GridBlock{8, 45, 3, 2}.GetRectMercator()}});
  auto downloader = std::make_unique<PendingTerrainDownloader>();
  auto & pending = *downloader;
  storage.SetDownloaderForTesting(std::move(downloader));
  storage.DownloadNode("East");
  pending.Start();
  pending.Progress(content.size());
  pending.Complete(content);
  TEST(pending.GetQueue().IsEmpty(), ());
  TEST_EQUAL(storage.CountryStatusEx("East"), Status::Downloading, ());
  west.Reset();
  terrain.Reset();
  storage.DeleteNode("West");
  bool const terrainPreserved = Platform::IsFileExistsByFullPath(terrain.GetFullPath());
  GetPlatform().RunTask(Platform::Thread::File,
                        [] { GetPlatform().RunTask(Platform::Thread::Gui, [] { testing::StopEventLoop(); }); });
  testing::RunEventLoop();
  NodeStatuses east;
  storage.GetNodeStatuses("East", east);
  TEST(terrainPreserved, (east.m_status));
  TEST_EQUAL(east.m_status, NodeStatus::OnDisk, ());
}
UNIT_CLASS_TEST(StorageTerrainDownloadTest, TerminalTerrainErrorsNotifyAfterPolicy)
{
  auto const content = TerrainHeaderContent();
  for (int failure = 0; failure < 3; ++failure)
  {
    tests_support::ScopedDirCleanup const terrainDir(base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR));
    tests_support::ScopedFile grid(TERRAIN_GRID_FILE, TerrainGridJson(content, failure != 1));
    StorageDownloadingPolicy policy;
    Storage storage(COUNTRIES_FILE);
    storage.SetDownloadingPolicy(&policy);
    storage.OnTerrainScanned({});
    auto downloader = std::make_unique<PendingTerrainDownloader>();
    auto & pending = *downloader;
    storage.SetDownloaderForTesting(std::move(downloader));
    storage.SetTerrainCallbacks([&](terrain::TwmFile const &) { return failure != 2; }, {});
    bool notified = false;
    bool terminalAtNotification = false;
    storage.Subscribe([&](CountryId const & id)
    {
      if (id != "Madagascar" || storage.GetTerrainAttrs(id).m_status != Storage::TerrainStatus::Failed)
        return;
      notified = true;
      terminalAtNotification = policy.IsTerrainAutoRetryDownloadFailed();
      if (failure != 0)
        testing::StopEventLoop();
    }, [](CountryId const &, storage::Progress const &) {});
    storage.DownloadTerrain("Madagascar");
    if (failure == 0)
      pending.Fail(downloader::DownloadStatus::FileNotFound);
    else
    {
      pending.Complete(content);
      testing::RunEventLoop();
    }
    TEST(notified, (failure));
    TEST(terminalAtNotification, (failure));
    storage.DownloadTerrain("Madagascar");
    TEST(!policy.IsTerrainAutoRetryDownloadFailed(), (failure));
    storage.CancelTerrain("Madagascar");
    TEST(!policy.IsTerrainAutoRetryDownloadFailed(), (failure));
  }
}

UNIT_CLASS_TEST(StorageTerrainDownloadTest, TerrainTerminalErrorsStayWithinTheirCoverage)
{
  tests_support::ScopedFile countries("terrain_mixed_failures.json", R"({"id":"Countries","v":260101,
    "g":[{"id":"West","s":100},{"id":"East","s":100},{"id":"Mixed","s":100}]})");
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, R"({"v":1,"blocks":[
    {"id":"N45E008","sx":3,"sy":2,"s":30,"h":"aA"},
    {"id":"N45E011","sx":3,"sy":2,"s":10,"h":"aA"}],
    "mwms":{"West":["N45E008"],"East":["N45E011"],"Mixed":["N45E008","N45E011"]}})");
  tests_support::ScopedDirCleanup const mapsDir(base::JoinPath(GetPlatform().WritableDir(), "260101"));
  tests_support::ScopedDirCleanup const terrainDir(base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR));
  tests_support::ScopedFile west("260101/West.mwm", std::string(100, 'm'));
  tests_support::ScopedFile east("260101/East.mwm", std::string(100, 'm'));
  tests_support::ScopedFile mixed("260101/Mixed.mwm", std::string(100, 'm'));
  for (bool const terminalFirst : {false, true})
  {
    StorageDownloadingPolicy policy;
    Storage storage("terrain_mixed_failures.json");
    storage.SetDownloadingPolicy(&policy);
    storage.RegisterAllLocalMaps();
    storage.OnTerrainScanned({});
    auto downloader = std::make_unique<PendingTerrainDownloader>();
    auto & pending = *downloader;
    storage.SetDownloaderForTesting(std::move(downloader));
    storage.DownloadTerrain("Mixed");
    storage.DownloadTerrain("West");
    storage.DownloadTerrain("East");
    TEST_EQUAL(pending.GetQueue().Count(), 2, ());
    pending.Fail(terminalFirst ? downloader::DownloadStatus::FileNotFound : downloader::DownloadStatus::Failed);
    pending.Fail(terminalFirst ? downloader::DownloadStatus::Failed : downloader::DownloadStatus::FileNotFound);

    auto const westAttrs = storage.GetTerrainAttrs("West");
    auto const eastAttrs = storage.GetTerrainAttrs("East");
    auto const mixedAttrs = storage.GetTerrainAttrs("Mixed");
    TEST_EQUAL(westAttrs.m_status, Storage::TerrainStatus::Failed, (terminalFirst));
    TEST_EQUAL(eastAttrs.m_status, Storage::TerrainStatus::Failed, (terminalFirst));
    TEST_EQUAL(westAttrs.m_error, terminalFirst ? NodeErrorCode::UnknownError : NodeErrorCode::NoInetConnection,
               (terminalFirst));
    TEST_EQUAL(eastAttrs.m_error, terminalFirst ? NodeErrorCode::NoInetConnection : NodeErrorCode::UnknownError,
               (terminalFirst));
    TEST_EQUAL(mixedAttrs.m_status, Storage::TerrainStatus::Failed, (terminalFirst));
    TEST_EQUAL(mixedAttrs.m_error, NodeErrorCode::UnknownError, (terminalFirst));
    NodeStatuses status;
    storage.GetNodeStatuses("Mixed", status);
    TEST_EQUAL(status.m_status, NodeStatus::Error, (terminalFirst));
    TEST_EQUAL(status.m_error, NodeErrorCode::UnknownError, (terminalFirst));
    TEST(policy.IsTerrainAutoRetryDownloadFailed(), (terminalFirst));
    TEST(!policy.IsTerrainTransportRetryFailed(), (terminalFirst));
    storage.CancelTerrain(storage.GetRootId());
    TEST(!policy.IsTerrainAutoRetryDownloadFailed(), (terminalFirst));
  }
}

UNIT_CLASS_TEST(StorageTerrainDownloadTest, TerrainRetryBeforeMapRegistration)
{
  auto const content = TerrainHeaderContent();
  tests_support::ScopedDirCleanup const terrainDir(base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR));
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, TerrainGridJson(content));
  RecordingDownloadingPolicy policy;
  Storage storage(COUNTRIES_FILE);
  storage.SetDownloadingPolicy(&policy);
  storage.OnTerrainScanned({});
  auto downloader = std::make_unique<PendingTerrainDownloader>();
  auto & pending = *downloader;
  storage.SetDownloaderForTesting(std::move(downloader));

  // Request ownership precedes map registration, and desktop can request terrain alone.
  storage.DownloadTerrain("Madagascar");
  pending.Fail(downloader::DownloadStatus::Failed);
  TEST_EQUAL(policy.m_calls, 1, ());
  TEST_EQUAL(policy.m_regions, (CountriesSet{"Madagascar"}), ());

  storage.DownloadTerrain("Madagascar");
  TEST_EQUAL(pending.m_requests, 2, ());
  TEST_EQUAL(storage.GetTerrainAttrs("Madagascar").m_status, Storage::TerrainStatus::Downloading, ());
  storage.CancelTerrain("Madagascar");
  TEST(pending.GetQueue().IsEmpty(), ());
  storage.RetryDownloadNode("Madagascar");
  TEST_EQUAL(pending.m_requests, 2, ());
}

UNIT_CLASS_TEST(StorageTerrainDownloadTest, CancelTerrainValidationCleansReadyFile)
{
  auto const content = TerrainHeaderContent();
  tests_support::ScopedDirCleanup const terrainDir(base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR));
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, TerrainGridJson(content));
  Storage storage(COUNTRIES_FILE);
  storage.OnTerrainScanned({});
  auto downloader = std::make_unique<PendingTerrainDownloader>();
  auto & pending = *downloader;
  storage.SetDownloaderForTesting(std::move(downloader));
  std::promise<void> release;
  auto released = release.get_future().share();
  GetPlatform().RunTask(Platform::Thread::File, [released] { released.wait(); });
  storage.DownloadTerrain("Madagascar");
  pending.Complete(content);
  storage.CancelTerrain("Madagascar");
  // Model platforms where the open validator prevents immediate unlinking.
  std::string const readyPath = base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR, "1", "N45E008.twm.ready");
  if (!Platform::IsFileExistsByFullPath(readyPath))
  {
    FileWriter writer(readyPath);
    writer.Write(content.data(), content.size());
  }
  release.set_value();
  GetPlatform().RunTask(Platform::Thread::File,
                        [] { GetPlatform().RunTask(Platform::Thread::Gui, [] { testing::StopEventLoop(); }); });
  testing::RunEventLoop();
  TEST(!Platform::IsFileExistsByFullPath(readyPath), ());
  TEST(!storage.IsDownloadInProgress(), ());
  TEST_EQUAL(storage.GetTerrainAttrs("Madagascar").m_status, Storage::TerrainStatus::NotDownloaded, ());
}
UNIT_TEST(Storage_TerrainOwnershipMigratesOnlyOnce)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  tests_support::ScopedFile countries("terrain_migration.json", R"({"id":"Countries","v":260101,
    "g":[{"id":"West","s":100},{"id":"East","s":200}]})");
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, R"({"v":2,"blocks":[
    {"id":"N45E008","sx":3,"sy":2,"s":30,"h":"aA"}],
    "mwms":{"West":["N45E008"],"East":["N45E008"]}})");
  tests_support::ScopedDir mapsDir("260101");
  tests_support::ScopedDir terrainDir(TERRAIN_DIR);
  tests_support::ScopedDir versionDir(terrainDir, "1");
  tests_support::ScopedFile west("260101/West.mwm", std::string(100, 'm'));
  tests_support::ScopedFile oldTerrain("terrain/1/N45E008.twm", "old terrain");
  std::vector<terrain::TwmFile> const files = {
      {"N45E008", 1, oldTerrain.GetFullPath(), terrain::GridBlock{8, 45, 3, 2}.GetRectMercator()}};
  {
    Storage storage("terrain_migration.json");
    storage.RegisterAllLocalMaps();
    storage.OnTerrainScanned(std::vector(files));
    std::string saved;
    TEST(settings::Get("TerrainRequestedRegions", saved), ());
    TEST_EQUAL(saved, "West", ());
  }
  tests_support::ScopedFile east("260101/East.mwm", std::string(200, 'm'));
  Storage storage("terrain_migration.json");
  storage.Init([](CountryId const &, LocalFilePtr const) {},
               [](CountryId const &, LocalFilePtr const) { return false; });
  storage.RegisterAllLocalMaps();
  storage.OnTerrainScanned(std::vector(files));
  // A subsequently downloaded map does not acquire an explicit terrain request.
  std::string saved;
  TEST(settings::Get("TerrainRequestedRegions", saved), ());
  TEST_EQUAL(saved, "West", ());
  storage.DeleteNode("West");
  west.Reset();
  TEST(oldTerrain.Exists(), ());  // East still uses the shared older terrain.
  storage.DeleteNode("East");
  east.Reset();
  TEST(!oldTerrain.Exists(), ());
  oldTerrain.Reset();
}

UNIT_TEST(Storage_MapAndTerrainDownloadingProgressAreIndependent)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  uint64_t constexpr kMegabyte = 1024 * 1024;
  tests_support::ScopedFile countries("active_downloads.json", R"({"id":"Countries","v":260101,
    "g":[{"id":"West","s":2097152}]})");
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, R"({"v":1,"blocks":[
    {"id":"N45E008","sx":3,"sy":2,"s":4194304,"h":"aA"}],
    "mwms":{"West":["N45E008"]}})");
  Storage storage("active_downloads.json");
  storage.OnTerrainScanned({});
  auto downloader = std::make_unique<PendingTerrainDownloader>();
  auto & pending = *downloader;
  storage.SetDownloaderForTesting(std::move(downloader));
  auto const check = [&](uint64_t mapDownloaded, uint64_t mapTotal, uint64_t terrainDownloaded, uint64_t terrainTotal)
  {
    auto const progress = storage.GetDownloadingProgress("West");
    TEST_EQUAL(progress.m_mwmDownloaded, static_cast<int64_t>(mapDownloaded), ());
    TEST_EQUAL(progress.m_mwmTotal, static_cast<int64_t>(mapTotal), ());
    TEST_EQUAL(progress.m_twmDownloaded, static_cast<int64_t>(terrainDownloaded), ());
    TEST_EQUAL(progress.m_twmTotal, static_cast<int64_t>(terrainTotal), ());
  };
  check(0, 0, 0, 0);
  storage.DownloadTerrain("West");
  check(0, 0, 0, 4 * kMegabyte);
  pending.Start();
  pending.Progress(kMegabyte);
  check(0, 0, kMegabyte, 4 * kMegabyte);
  storage.DownloadNode("West");
  check(0, 2 * kMegabyte, kMegabyte, 4 * kMegabyte);
  storage.CancelTerrain("West");
  check(0, 2 * kMegabyte, 0, 0);
  pending.Start();
  pending.Progress(kMegabyte);
  check(kMegabyte, 2 * kMegabyte, 0, 0);
  storage.CancelDownloadNode("West");
  check(0, 0, 0, 0);

  tests_support::ScopedDir mapsDir("260101");
  tests_support::ScopedFile west("260101/West.mwm", std::string(2 * kMegabyte, 'm'));
  storage.RegisterAllLocalMaps();
  check(0, 0, 0, 0);
  storage.DownloadTerrain("West");
  // The installed MWM does not acquire progress from the terrain request.
  check(0, 0, 0, 4 * kMegabyte);
  storage.CancelTerrain("West");
  check(0, 0, 0, 0);
}

UNIT_CLASS_TEST(StorageTerrainDownloadTest, TerrainFolderKeepsCompletedOwnerProgress)
{
  auto const westContent = TerrainHeaderContent();
  auto const eastContent = TerrainHeaderContent({11, 45, 3, 2});
  tests_support::ScopedFile countries("terrain_folder_progress.json", R"({"id":"Countries","v":260101,
    "g":[{"id":"Slovenia","g":[{"id":"Slovenia_West","s":100},{"id":"Slovenia_East","s":200}]}]})");
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, R"({"v":1,"blocks":[
    {"id":"N45E008","sx":3,"sy":2,"s":)" + strings::to_string(westContent.size()) +
                                                        R"(,"h":"aA"},
    {"id":"N45E011","sx":3,"sy":2,"s":)" + strings::to_string(eastContent.size()) +
                                                        R"(,"h":"aA"}],
    "mwms":{"Slovenia_West":["N45E008"],"Slovenia_East":["N45E011"]}})");
  tests_support::ScopedDirCleanup const mapsDir(base::JoinPath(GetPlatform().WritableDir(), "260101"));
  tests_support::ScopedDirCleanup const terrainDir(base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR));
  tests_support::ScopedFile west("260101/Slovenia_West.mwm", std::string(100, 'm'));
  tests_support::ScopedFile east("260101/Slovenia_East.mwm", std::string(200, 'm'));
  Storage storage("terrain_folder_progress.json");
  storage.SetEnabledIntegrityValidationForTesting(false);
  storage.RegisterAllLocalMaps();
  storage.OnTerrainScanned({});
  storage.SetTerrainCallbacks([](terrain::TwmFile const &)
  {
    testing::StopEventLoop();
    return true;
  }, {});
  auto downloader = std::make_unique<PendingTerrainDownloader>();
  auto & pending = *downloader;
  storage.SetDownloaderForTesting(std::move(downloader));
  storage.DownloadTerrain("Slovenia");
  TEST_EQUAL(pending.GetQueue().Count(), 2, ());
  pending.Start();
  pending.Progress(westContent.size());
  auto const before = storage.GetTerrainAttrs("Slovenia").m_downloadingProgress;
  pending.Complete(westContent);
  testing::RunEventLoop();
  auto const after = storage.GetTerrainAttrs("Slovenia").m_downloadingProgress;
  TEST_EQUAL(storage.GetTerrainAttrs("Slovenia").m_status, Storage::TerrainStatus::Downloading, ());
  TEST_EQUAL(after.m_bytesTotal, before.m_bytesTotal, (before.m_bytesTotal, after.m_bytesTotal));
  TEST_EQUAL(after.m_bytesDownloaded, before.m_bytesDownloaded, (before.m_bytesDownloaded, after.m_bytesDownloaded));
  auto const rootProgress = storage.GetTerrainAttrs(storage.GetRootId()).m_downloadingProgress;
  TEST_EQUAL(rootProgress.m_bytesDownloaded, after.m_bytesDownloaded, ());
  TEST_EQUAL(rootProgress.m_bytesTotal, after.m_bytesTotal, ());
  TEST_EQUAL(storage.GetDownloadingProgress(storage.GetRootId()).m_twmDownloaded, after.m_bytesDownloaded, ());
  TEST_EQUAL(storage.GetDownloadingProgress(storage.GetRootId()).m_twmTotal, after.m_bytesTotal, ());
  pending.Start();
  pending.Complete(eastContent);
  testing::RunEventLoop();
  TEST_EQUAL(storage.GetTerrainAttrs("Slovenia").m_status, Storage::TerrainStatus::OnDisk, ());
  TEST_EQUAL(storage.GetDownloadingProgress("Slovenia").m_twmTotal, 0, ());
  TEST_EQUAL(storage.GetDownloadingProgress(storage.GetRootId()).m_twmTotal, 0, ());
  // A fresh batch must not include regions completed by the previous batch.
  storage.DeleteTerrain("Slovenia_West");
  storage.DownloadTerrain("Slovenia_West");
  for (auto const & id : {CountryId("Slovenia"), storage.GetRootId()})
  {
    auto const progress = storage.GetTerrainAttrs(id).m_downloadingProgress;
    TEST_EQUAL(progress.m_bytesDownloaded, 0, (id));
    TEST_EQUAL(progress.m_bytesTotal, static_cast<int64_t>(westContent.size()), (id));
  }
  storage.CancelTerrain("Slovenia_West");
}

UNIT_TEST(Storage_MapProgressKeepsLatestBytes)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  tests_support::ScopedFile countries("map_progress_notifications.json", R"({"id":"Countries","v":260101,
    "g":[{"id":"Group","g":[{"id":"West","s":1048576}]}]})");
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, R"({"v":1,"blocks":[],"mwms":{}})");
  tests_support::ScopedDirCleanup const mapsDir(base::JoinPath(GetPlatform().WritableDir(), "260101"));
  Storage storage("map_progress_notifications.json");
  auto downloader = std::make_unique<PendingTerrainDownloader>();
  auto & pending = *downloader;
  storage.SetDownloaderForTesting(std::move(downloader));
  std::vector<storage::Progress> notifications;
  auto const onProgress = [&](CountryId const & id, storage::Progress const & p)
  {
    if (id == "West")
      notifications.push_back(p);
  };
  auto slot = storage.Subscribe([](auto const &) {}, onProgress);
  storage.DownloadNode("West");
  pending.Start();
  TEST_EQUAL(notifications.size(), 1, ());
  TEST_EQUAL(notifications.front().m_mwmDownloaded, 0, ());
  TEST_EQUAL(notifications.front().m_mwmTotal, 1048576, ());
  pending.Progress(1);
  TEST_EQUAL(notifications.size(), 2, ());
  TEST_EQUAL(notifications.back().m_mwmDownloaded, 1, ());

  uint64_t constexpr kSize = 1024 * 1024;
  pending.Progress(kSize / 2);
  TEST_EQUAL(notifications.back().m_mwmDownloaded, kSize / 2, ());
  storage.Unsubscribe(slot);
  // The latest bytes remain available while no UI observes the download.
  pending.Progress(kSize - 1024);
  NodeAttrs attrs;
  storage.GetNodeAttrs("West", attrs);
  TEST_EQUAL(attrs.m_downloadingProgress.m_bytesDownloaded, kSize - 1024, ());
  TEST_EQUAL(storage.GetDownloadingProgress("Group").m_mwmDownloaded, kSize - 1024, ());
  TEST_EQUAL(storage.GetDownloadingProgress(storage.GetRootId()).m_twmTotal, 0, ());
  slot = storage.Subscribe([](auto const &) {}, onProgress);
  pending.Progress(kSize);
  TEST_EQUAL(notifications.back().m_mwmDownloaded, kSize, ());
  TEST_EQUAL(notifications.back().m_mwmTotal, kSize, ());

  // A cancelled attempt does not affect a restarted request's first update.
  storage.CancelDownloadNode("West");
  notifications.clear();
  storage.DownloadNode("West");
  pending.Start();
  TEST_EQUAL(notifications.size(), 1, ());
  TEST_EQUAL(notifications.front().m_mwmDownloaded, 0, ());
  TEST_EQUAL(notifications.front().m_mwmTotal, 1048576, ());
  pending.Progress(1);
  TEST_EQUAL(notifications.size(), 2, ());
  TEST_EQUAL(notifications.back().m_mwmDownloaded, 1, ());
  storage.CancelDownloadNode("West");
  storage.Unsubscribe(slot);
}

UNIT_TEST(Storage_ConcurrentMapProgressWithUnknownTransportSize)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  tests_support::ScopedFile countries("concurrent_map_progress.json", R"({"id":"Countries","v":260101,
    "g":[{"id":"Group","g":[{"id":"West","s":1048576},{"id":"East","s":1048576}]}]})");
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, R"({"v":1,"blocks":[],"mwms":{}})");
  Storage storage("concurrent_map_progress.json");
  storage.SetDownloaderForTesting(std::make_unique<PendingTerrainDownloader>());
  auto diffs = std::make_shared<diffs::DiffsDataSource>();
  QueuedCountry west(storage.GetCountryFile("West"), "West", MapFileType::Map, storage.GetCurrentDataVersion(), {},
                     diffs);
  QueuedCountry east(storage.GetCountryFile("East"), "East", MapFileType::Map, storage.GetCurrentDataVersion(), {},
                     diffs);
  west.Subscribe(storage);
  east.Subscribe(storage);
  std::map<CountryId, std::vector<storage::Progress>> notifications;
  auto const slot = storage.Subscribe(
      [](auto const &) {}, [&](CountryId const & id, storage::Progress const & p) { notifications[id].push_back(p); });
  // iOS background downloads can start together and omit Content-Length.
  west.OnStartDownloading();
  east.OnStartDownloading();
  for (auto const * id : {"West", "East"})
  {
    TEST_EQUAL(notifications[id].size(), 1, (id));
    TEST_EQUAL(notifications[id].front().m_mwmDownloaded, 0, (id));
    TEST_EQUAL(notifications[id].front().m_mwmTotal, 1048576, (id));
  }
  west.OnDownloadProgress({1, downloader::Progress::kUnknownTotalSize});
  east.OnDownloadProgress({1, downloader::Progress::kUnknownTotalSize});
  for (auto const * id : {"West", "East"})
  {
    TEST_EQUAL(notifications[id].size(), 2, (id));
    TEST_EQUAL(notifications[id].back().m_mwmDownloaded, 1, (id));
    TEST_EQUAL(notifications[id].back().m_mwmTotal, 1048576, (id));
  }
  for (int64_t i = 2; i < 10; ++i)
  {
    west.OnDownloadProgress({i, downloader::Progress::kUnknownTotalSize});
    east.OnDownloadProgress({i, downloader::Progress::kUnknownTotalSize});
  }
  for (auto const * id : {"West", "East"})
    TEST_EQUAL(notifications[id].size(), 10, (id));
  auto const progress = storage.GetDownloadingProgress("Group");
  TEST_EQUAL(progress.m_mwmDownloaded, 18, ());
  TEST_EQUAL(progress.m_mwmTotal, 2097152, ());
  west.OnDownloadProgress({1048576, downloader::Progress::kUnknownTotalSize});
  east.OnDownloadProgress({1048576, downloader::Progress::kUnknownTotalSize});
  for (auto const * id : {"West", "East"})
    TEST_EQUAL(notifications[id].back().m_mwmDownloaded, 1048576, (id));
  storage.CancelDownloadNode(storage.GetRootId());
  storage.Unsubscribe(slot);
}

UNIT_TEST(Storage_ActiveMapProgressDeduplicatesAndKeepsCompletedScope)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  tests_support::ScopedFile countries("active_map_progress.json", R"({"id":"Countries","v":260101,
    "g":[{"id":"Group","g":[{"id":"West","s":100},{"id":"Shared","s":200}]},
         {"id":"Other","g":[{"id":"Shared","s":200},{"id":"East","s":300}]},
         {"id":"Installed","s":400}]})");
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, R"({"v":1,"blocks":[],"mwms":{}})");
  tests_support::ScopedDirCleanup const mapsDir(base::JoinPath(GetPlatform().WritableDir(), "260101"));
  tests_support::ScopedFile installed("260101/Installed.mwm", std::string(400, 'm'));
  Storage storage("active_map_progress.json");
  storage.Init([](auto const &, auto const &) {}, [](auto const &, auto const &) { return false; });
  storage.SetEnabledIntegrityValidationForTesting(false);
  storage.RegisterAllLocalMaps();
  auto downloader = std::make_unique<PendingTerrainDownloader>();
  auto & pending = *downloader;
  storage.SetDownloaderForTesting(std::move(downloader));
  std::map<CountryId, storage::Progress> received;
  auto const slot = storage.Subscribe([](auto const &) {},
                                      [&](CountryId const & id, storage::Progress const & p) { received[id] = p; });
  auto const check = [&](CountryId const & id, int64_t downloaded, int64_t total)
  {
    TEST_EQUAL(received.at(id).m_mwmDownloaded, downloaded, (id));
    TEST_EQUAL(received.at(id).m_mwmTotal, total, (id));
    auto const current = storage.GetDownloadingProgress(id);
    TEST_EQUAL(current.m_mwmDownloaded, downloaded, (id));
    TEST_EQUAL(current.m_mwmTotal, total, (id));
  };
  storage.DownloadNode("Group");
  storage.DownloadNode("Other");
  pending.Start();
  pending.Progress(50);
  check("Group", 50, 300);
  check(storage.GetRootId(), 50, 600);
  pending.Complete(std::string(100, 'm'));
  pending.Start();
  pending.Progress(100);
  // Shared occurs under both parents but contributes to root only once.
  check("Shared", 100, 200);
  check("Group", 200, 300);
  check("Other", 100, 500);
  check(storage.GetRootId(), 200, 600);
  pending.Complete(std::string(200, 'm'));
  pending.Start();
  pending.Progress(150);
  check("Other", 350, 500);
  check(storage.GetRootId(), 450, 600);
  TEST_EQUAL(storage.GetDownloadingProgress("Group").m_mwmTotal, 0, ());
  NodeAttrs attrs;
  storage.GetNodeAttrs("Group", attrs);
  TEST_EQUAL(attrs.m_status, NodeStatus::OnDisk, ());
  TEST_EQUAL(attrs.m_downloadingProgress.m_bytesTotal, 300, ());
  pending.Complete(std::string(300, 'm'));
  TEST_EQUAL(storage.GetDownloadingProgress(storage.GetRootId()).m_mwmTotal, 0, ());
  storage.Unsubscribe(slot);
}

UNIT_TEST(Storage_MapAndTerrainProgressUsesCountrySnapshots)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  tests_support::ScopedFile countries("country_progress.json", R"({"id":"Countries","v":260101,
    "g":[{"id":"Group","g":[{"id":"West","s":100}]}]})");
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, R"({"v":1,"blocks":[
    {"id":"N45E008","sx":3,"sy":2,"s":2097152,"h":"aA"}],"mwms":{"West":["N45E008"]}})");
  tests_support::ScopedDirCleanup const mapsDir(base::JoinPath(GetPlatform().WritableDir(), "260101"));
  tests_support::ScopedDirCleanup const terrainDir(base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR));
  Storage storage("country_progress.json");
  storage.OnTerrainScanned({});
  auto downloader = std::make_unique<PendingTerrainDownloader>();
  auto & pending = *downloader;
  storage.SetDownloaderForTesting(std::move(downloader));
  CountriesSet const hierarchy = {"West", "Group", storage.GetRootId()};
  std::map<CountryId, std::vector<storage::Progress>> notifications;
  auto const slot = storage.Subscribe([](auto const &) {}, [&](CountryId const & id, storage::Progress const & progress)
  {
    TEST(hierarchy.contains(id), (id));
    auto const current = storage.GetDownloadingProgress(id);
    TEST_EQUAL(progress.m_mwmDownloaded, current.m_mwmDownloaded, (id));
    TEST_EQUAL(progress.m_mwmTotal, current.m_mwmTotal, (id));
    TEST_EQUAL(progress.m_twmDownloaded, current.m_twmDownloaded, (id));
    TEST_EQUAL(progress.m_twmTotal, current.m_twmTotal, (id));
    notifications[id].push_back(progress);
  });
  storage.DownloadCountry("West", MapFileType::Map);
  pending.Start();
  pending.Progress(1);
  auto map = static_cast<Queue const &>(pending.GetQueue()).GetFirstCountry();
  map.OnDownloadProgress({7, downloader::Progress::kUnknownTotalSize});
  for (auto const & id : hierarchy)
  {
    TEST_EQUAL(notifications[id].size(), 3, (id));
    for (size_t i = 0; i < notifications[id].size(); ++i)
    {
      auto const & progress = notifications[id][i];
      TEST_EQUAL(progress.m_mwmDownloaded, (std::array<int64_t, 3>{0, 1, 7}[i]), (id));
      TEST_EQUAL(progress.m_mwmTotal, 100, (id));
      TEST_EQUAL(progress.m_twmDownloaded, 0, (id));
      TEST_EQUAL(progress.m_twmTotal, 0, (id));
    }
  }
  storage.CancelDownloadNode("West");
  storage.DownloadTerrain("West");
  notifications.clear();
  pending.Start();
  pending.Progress(1);
  auto tile = static_cast<Queue const &>(pending.GetQueue()).GetFirstCountry();
  tile.OnDownloadProgress({7, downloader::Progress::kUnknownTotalSize});
  for (auto const & id : hierarchy)
  {
    TEST_EQUAL(notifications[id].size(), 1, (id));
    auto const & progress = notifications[id].front();
    TEST_EQUAL(progress.m_mwmDownloaded, 0, (id));
    TEST_EQUAL(progress.m_mwmTotal, 0, (id));
    TEST_EQUAL(progress.m_twmDownloaded, 0, (id));
    TEST_EQUAL(progress.m_twmTotal, 2097152, (id));
    // Below the terrain notification threshold, polling still gives the latest bytes.
    TEST_EQUAL(storage.GetDownloadingProgress(id).m_twmDownloaded, 7, (id));
  }
  pending.Progress(1024 * 1024);
  for (auto const & id : hierarchy)
  {
    TEST_EQUAL(notifications[id].size(), 2, (id));
    auto const & progress = notifications[id].back();
    TEST_EQUAL(progress.m_mwmTotal, 0, (id));
    TEST_EQUAL(progress.m_twmDownloaded, 1024 * 1024, (id));
    TEST_EQUAL(progress.m_twmTotal, 2097152, (id));
  }
  storage.CancelTerrain("West");
  storage.Unsubscribe(slot);
}

UNIT_TEST(Storage_ConcurrentMapAndTerrainProgressSnapshot)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  tests_support::ScopedFile countriesFile("concurrent_country_progress.json", R"({"id":"Countries","v":260101,
    "g":[{"id":"Group","g":[{"id":"West","s":100}]}]})");
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, R"({"v":1,"blocks":[
    {"id":"N45E008","sx":3,"sy":2,"s":2097152,"h":"aA"}],"mwms":{"West":["N45E008"]}})");
  tests_support::ScopedDirCleanup const mapsDir(base::JoinPath(GetPlatform().WritableDir(), "260101"));
  tests_support::ScopedDirCleanup const terrainDir(base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR));
  auto downloader = std::make_unique<PendingTerrainDownloader>();
  auto & pending = *downloader;
  Storage storage("concurrent_country_progress.json");
  storage.SetDownloaderForTesting(std::move(downloader));
  storage.OnTerrainScanned({});
  storage.DownloadNode("West");
  pending.Start();
  pending.Progress(7);
  storage.DownloadTerrain("West");
  std::unique_ptr<QueuedCountry> tile;
  pending.GetQueue().ForEachCountry([&](QueuedCountry const & country)
  {
    if (country.GetFileType() == MapFileType::Terrain)
      tile = std::make_unique<QueuedCountry>(country);
  });
  TEST(tile, ());
  std::map<CountryId, std::vector<storage::Progress>> notifications;
  auto const slot = storage.Subscribe([](auto const &) {}, [&](CountryId const & id, storage::Progress const & p)
  {
    auto const current = storage.GetDownloadingProgress(id);
    TEST_EQUAL(p.m_mwmDownloaded, current.m_mwmDownloaded, (id));
    TEST_EQUAL(p.m_mwmTotal, current.m_mwmTotal, (id));
    TEST_EQUAL(p.m_twmDownloaded, current.m_twmDownloaded, (id));
    TEST_EQUAL(p.m_twmTotal, current.m_twmTotal, (id));
    notifications[id].push_back(p);
  });
  tile->OnStartDownloading();
  tile->OnDownloadProgress({1024 * 1024, downloader::Progress::kUnknownTotalSize});
  CountriesSet countries;
  for (auto const & [id, events] : notifications)
  {
    countries.insert(id);
    TEST_EQUAL(events.size(), 2, (id));
    for (size_t i = 0; i < events.size(); ++i)
    {
      TEST_EQUAL(events[i].m_mwmDownloaded, 7, (id));
      TEST_EQUAL(events[i].m_mwmTotal, 100, (id));
      TEST_EQUAL(events[i].m_twmDownloaded, i == 0 ? 0 : 1024 * 1024, (id));
      TEST_EQUAL(events[i].m_twmTotal, 2097152, (id));
    }
  }
  TEST_EQUAL(countries, (CountriesSet{"West", "Group", storage.GetRootId()}), ());
  storage.CancelDownloadNode(storage.GetRootId());
  storage.CancelTerrain(storage.GetRootId());
  storage.Unsubscribe(slot);
}

UNIT_TEST(Storage_MapProgressUsesRequestedFileSize)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  tests_support::ScopedDirCleanup const mapsDir(base::JoinPath(GetPlatform().WritableDir(), "260101"));
  auto downloader = std::make_unique<PendingTerrainDownloader>();
  auto & pending = *downloader;
  Storage storage(R"({"id":"Countries","v":260101,"g":[{"id":"West","s":100}]})", std::move(downloader));
  storage.SetDiffInfoForTesting({{"West", {10, 260100}}});
  std::vector<storage::Progress> notifications;
  auto const slot = storage.Subscribe([](auto const &) {}, [&](CountryId const & id, storage::Progress const & progress)
  {
    if (id == "West")
      notifications.push_back(progress);
  });
  for (auto const type : {MapFileType::Map, MapFileType::Diff})
  {
    notifications.clear();
    storage.DownloadCountry("West", type);
    auto const size = type == MapFileType::Map ? 100 : 10;
    auto progress = storage.GetDownloadingProgress("West");
    TEST_EQUAL(progress.m_mwmDownloaded, 0, (type));
    TEST_EQUAL(progress.m_mwmTotal, size, (type));
    pending.Start();
    progress = storage.GetDownloadingProgress("West");
    TEST_EQUAL(progress.m_mwmDownloaded, 0, (type));
    TEST_EQUAL(progress.m_mwmTotal, size, (type));
    auto country = static_cast<Queue const &>(pending.GetQueue()).GetFirstCountry();
    country.OnDownloadProgress({7, downloader::Progress::kUnknownTotalSize});
    progress = storage.GetDownloadingProgress("West");
    TEST_EQUAL(progress.m_mwmDownloaded, 7, (type));
    TEST_EQUAL(progress.m_mwmTotal, size, (type));
    TEST_EQUAL(notifications.size(), 2, (type));
    for (size_t i = 0; i < notifications.size(); ++i)
    {
      auto const & progress = notifications[i];
      TEST_EQUAL(progress.m_mwmDownloaded, i == 0 ? 0 : 7, (type));
      TEST_EQUAL(progress.m_mwmTotal, size, (type));
      TEST_EQUAL(progress.m_twmDownloaded, 0, (type));
      TEST_EQUAL(progress.m_twmTotal, 0, (type));
    }
    storage.CancelDownloadNode("West");
  }
  storage.Unsubscribe(slot);
}

UNIT_TEST(Storage_MapAndTerrainProgressCanCancel)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  tests_support::ScopedFile countries("country_cancel.json", R"({"id":"Countries","v":260101,
    "g":[{"id":"Group","g":[{"id":"West","s":2097152}]}]})");
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, R"({"v":1,"blocks":[
    {"id":"N45E008","sx":3,"sy":2,"s":2097152,"h":"aA"}],"mwms":{"West":["N45E008"]}})");
  int64_t constexpr kSize = 2 * 1024 * 1024;
  int64_t constexpr kOtherDownloaded = 256;
  for (bool const terrain : {false, true})
    for (bool const cancelOnStart : {false, true})
      for (bool const keepOtherFlow : {false, true})
      {
        tests_support::ScopedDirCleanup const mapsDir(base::JoinPath(GetPlatform().WritableDir(), "260101"));
        tests_support::ScopedDirCleanup const terrainDir(base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR));
        Storage storage("country_cancel.json");
        storage.OnTerrainScanned({});
        auto downloader = std::make_unique<PendingTerrainDownloader>();
        auto & pending = *downloader;
        storage.SetDownloaderForTesting(std::move(downloader));
        if (keepOtherFlow)
        {
          if (terrain)
            storage.DownloadCountry("West", MapFileType::Map);
          else
            storage.DownloadTerrain("West");
          pending.Start();
          pending.Progress(kOtherDownloaded);
        }
        int slot = 0;
        bool cancelled = false;
        size_t cancellationNotifications = 0;
        std::map<CountryId, storage::Progress> received;
        auto const secondSlot =
            storage.Subscribe([](auto const &) {}, [&](CountryId const & id, storage::Progress const & progress)
        {
          auto const current = storage.GetDownloadingProgress(id);
          TEST_EQUAL(progress.m_mwmDownloaded, current.m_mwmDownloaded, (id));
          TEST_EQUAL(progress.m_mwmTotal, current.m_mwmTotal, (id));
          TEST_EQUAL(progress.m_twmDownloaded, current.m_twmDownloaded, (id));
          TEST_EQUAL(progress.m_twmTotal, current.m_twmTotal, (id));
          received[id] = progress;
          if (cancelled)
            TEST_EQUAL(terrain ? progress.m_twmTotal : progress.m_mwmTotal, 0, (id));
        });
        slot = storage.Subscribe([](auto const &) {}, [&](CountryId const & id, storage::Progress const & progress)
        {
          ++cancellationNotifications;
          if (id != "West")
            return;
          auto const downloaded = terrain ? progress.m_twmDownloaded : progress.m_mwmDownloaded;
          if (downloaded != (cancelOnStart ? 0 : 1024 * 1024))
            return;
          cancelled = true;
          if (terrain)
            storage.CancelTerrain("West");
          else
            storage.CancelDownloadNode("West");
          storage.Unsubscribe(slot);
        });
        if (terrain)
          storage.DownloadTerrain("West");
        else
          storage.DownloadCountry("West", MapFileType::Map);
        std::unique_ptr<QueuedCountry> request;
        pending.GetQueue().ForEachCountry([&](QueuedCountry const & country)
        {
          if (country.GetFileType() == (terrain ? MapFileType::Terrain : MapFileType::Map))
            request = std::make_unique<QueuedCountry>(country);
        });
        TEST(request, (terrain, cancelOnStart, keepOtherFlow));
        request->OnStartDownloading();
        if (!cancelOnStart)
          request->OnDownloadProgress({1024 * 1024, downloader::Progress::kUnknownTotalSize});
        TEST(cancelled, (terrain, cancelOnStart, keepOtherFlow));
        // Unsubscribing in the leaf callback excludes the cancelling observer from subsequent ancestors.
        TEST_EQUAL(cancellationNotifications, cancelOnStart ? 1 : 4, (terrain, keepOtherFlow));
        bool const otherRemains = terrain && keepOtherFlow;
        TEST_EQUAL(pending.GetQueue().Count(), otherRemains ? 1 : 0, (terrain, cancelOnStart));
        TEST_EQUAL(storage.IsDownloadInProgress(), otherRemains, (terrain, cancelOnStart));
        for (auto const & id : {CountryId("Group"), storage.GetRootId()})
        {
          TEST(received.contains(id), (id));
          auto const & progress = received.at(id);
          TEST_EQUAL(terrain ? progress.m_twmDownloaded : progress.m_mwmDownloaded, 0, (id));
          TEST_EQUAL(terrain ? progress.m_twmTotal : progress.m_mwmTotal, 0, (id));
          TEST_EQUAL(terrain ? progress.m_mwmDownloaded : progress.m_twmDownloaded, otherRemains ? kOtherDownloaded : 0,
                     (id));
          TEST_EQUAL(terrain ? progress.m_mwmTotal : progress.m_twmTotal, otherRemains ? kSize : 0, (id));
        }
        storage.CancelDownloadNode("West");
        storage.CancelTerrain("West");
        storage.Unsubscribe(secondSlot);
      }
}

UNIT_TEST(Storage_RegionDeletionRemovesExclusiveTerrainAndOwnership)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  tests_support::ScopedFile countries("terrain_region_deletion.json", R"({"id":"Countries","v":260101,
    "g":[{"id":"West","s":100},{"id":"East","s":200}]})");
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, R"({"v":1,"blocks":[
    {"id":"N45E008","sx":3,"sy":2,"s":30,"h":"aA"},
    {"id":"N45E011","sx":3,"sy":2,"s":10,"h":"aA"}],
    "mwms":{"West":["N45E008","N45E011"],"East":["N45E008"]}})");
  tests_support::ScopedDir mapsDir("260101");
  tests_support::ScopedDir terrainDir(TERRAIN_DIR);
  tests_support::ScopedDir versionDir(terrainDir, "1");
  tests_support::ScopedFile west("260101/West.mwm", std::string(100, 'm'));
  tests_support::ScopedFile east("260101/East.mwm", std::string(200, 'm'));
  tests_support::ScopedFile shared("terrain/1/N45E008.twm", std::string(30, 't'));
  tests_support::ScopedFile exclusive("terrain/1/N45E011.twm", std::string(10, 't'));
  Storage storage("terrain_region_deletion.json");
  storage.Init([](CountryId const &, LocalFilePtr const) {},
               [](CountryId const &, LocalFilePtr const) { return false; });
  storage.RegisterAllLocalMaps();
  storage.OnTerrainScanned({});
  storage.OnTerrainScanned(
      {{"N45E008", 1, shared.GetFullPath(), terrain::GridBlock{8, 45, 3, 2}.GetRectMercator()},
       {"N45E011", 1, exclusive.GetFullPath(), terrain::GridBlock{11, 45, 3, 2}.GetRectMercator()}});
  storage.DownloadTerrain("West");
  storage.DownloadTerrain("East");
  storage.DeleteNode("West");
  west.Reset();
  TEST(!west.Exists(), ());
  TEST(shared.Exists(), ());
  TEST(!exclusive.Exists(), (exclusive.GetFullPath()));
  exclusive.Reset();
  std::string requested;
  TEST(settings::Get("TerrainRequestedRegions", requested), ());
  TEST_EQUAL(requested, "East", ());
  storage.DeleteNode("East");
  east.Reset();
  TEST(!east.Exists(), ());
  TEST(!shared.Exists(), (shared.GetFullPath()));
  shared.Reset();
  TEST(settings::Get("TerrainRequestedRegions", requested), ());
  TEST(requested.empty(), (requested));
}

UNIT_TEST(Storage_RegionDeletionCancelsOnlyUnsharedTerrain)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  tests_support::ScopedFile countries("terrain_folder.json", kFolderCountries);
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, kFolderTerrainGrid);
  tests_support::ScopedDir mapsDir("260101");
  tests_support::ScopedFile west("260101/West.mwm", std::string(100, 'm'));
  tests_support::ScopedFile east("260101/East.mwm", std::string(200, 'm'));
  tests_support::ScopedDirCleanup const terrainDir(base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR));
  Storage storage("terrain_folder.json");
  storage.Init([](CountryId const &, LocalFilePtr const) {},
               [](CountryId const &, LocalFilePtr const) { return false; });
  storage.RegisterAllLocalMaps();
  storage.OnTerrainScanned({});
  auto downloader = std::make_unique<PendingTerrainDownloader>();
  auto const & queue = downloader->GetQueue();
  storage.SetDownloaderForTesting(std::move(downloader));
  storage.DownloadTerrain("West");
  storage.DownloadTerrain("East");
  TEST_EQUAL(queue.Count(), 3, ());
  storage.DeleteNode("West");
  west.Reset();
  TEST(!west.Exists(), ());
  TEST(queue.Contains("N45E008") && queue.Contains("N45E014"), ());
  TEST(!queue.Contains("N45E011"), ());
  TEST_EQUAL(storage.GetTerrainAttrs("East").m_status, Storage::TerrainStatus::Downloading, ());
  storage.DeleteNode("East");
  east.Reset();
  TEST(queue.IsEmpty(), ());
  TEST(!storage.IsDownloadInProgress(), ());
  std::string requested;
  TEST(settings::Get("TerrainRequestedRegions", requested), ());
  TEST(requested.empty(), (requested));
}

}  // namespace storage_terrain_tests
