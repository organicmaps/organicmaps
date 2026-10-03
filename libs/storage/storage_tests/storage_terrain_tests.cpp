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
    for (auto const * key : {"DownloadQueue", "TerrainDownloadQueue"})
    {
      std::string value;
      if (settings::Get(key, value))
        m_saved.emplace(key, std::move(value));
      settings::Delete(key);
    }
  }

  ~ScopedDownloadQueue()
  {
    for (auto const * key : {"DownloadQueue", "TerrainDownloadQueue"})
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
  storage.OnTerrainScanned(scanned);
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

std::string TerrainHeaderContent(terrain::GridBlock const & block = {8, 45, 3, 2})
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
  }, [](CountryId const &, downloader::Progress const &) {});
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
      if (mapOnDisk)
        storage.DownloadNode("Madagascar");
      else
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
      storage.CancelDownloadNode("Madagascar");
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
      storage.CancelDownloadNode("Antarctica");
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
      storage.CancelDownloadNode("Antarctica");
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

  // The downloader UI queries the terrain attrs of every row: every node, the groups
  // via the deduplicated union of their leafs, must resolve without a hiccup. Every
  // downloadable leaf must have coverage in data/twm_grid.json (the bundle guard: the
  // grid and data/borders drift independently), World/WorldCoasts must have none.
  size_t checked = 0;
  storage.ForEachInSubtree(storage.GetRootId(), [&](CountryId const & id, bool groupNode)
  {
    auto const attrs = storage.GetTerrainAttrs(id);
    if (!groupNode && id != WORLD_FILE_NAME && id != WORLD_COASTS_FILE_NAME)
      TEST_NOT_EQUAL(attrs.m_status, Storage::TerrainStatus::NotAvailable, (id));
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
  storage.OnTerrainScanned(files);

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
  Storage::ParseTwmGridJson(R"({"v": 260729, "blocks": [
      {"id": "N45E008", "sx": 3, "sy": 2, "s": 10, "h": "aGFzaDE"},
      {"id": "N45E011", "sx": 2, "sy": 2, "s": 20, "h": "aGFzaDI", "v": 260315}],
      "mwms": {"A": ["N45E011", "N45E008", "N45E011"], "B": ["N45E008"]}})",
                            version, blocks, coverage);
  TEST_EQUAL(version, 260729, ());
  TEST_EQUAL(blocks.size(), 2, ());
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
  storage.OnTerrainScanned(files);
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

  storage.CancelDownloadNode(region);
  TEST_EQUAL(storage.GetTerrainAttrs(region).m_status, Storage::TerrainStatus::NotDownloaded, (region));
  TEST(!ready.Exists(), ());
  TEST(!resume.Exists(), ());
  ready.Reset();
  resume.Reset();
}

UNIT_TEST(Storage_TerrainUpdateInfo)
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

  // A block that is the WHOLE coverage of several regions: one file on disk completes
  // them all, so losing it leaves every one of them missing the very same block - which is
  // what the dedup has to collapse into a single size.
  std::map<uint32_t, std::vector<CountryId>> soleOwners;
  for (auto const & [region, indices] : coverage)
    if (indices.size() == 1)
      soleOwners[indices.front()].push_back(region);
  auto const it =
      std::find_if(soleOwners.begin(), soleOwners.end(), [](auto const & entry) { return entry.second.size() > 1; });
  TEST(it != soleOwners.end(), ());
  auto const & block = blocks[it->first];

  // The terrain follows the maps: two downloaded co-owner regions, no terrain files -
  // the shared block is missing for both. The fake maps carry the CURRENT data
  // version, an older one would read as out-of-date maps and pollute the counters.
  Storage storage;
  tests_support::ScopedDir mapsDir(strings::to_string(storage.GetCurrentDataVersion()));
  tests_support::ScopedFile map1(mapsDir, platform::CountryFile(it->second[0]), MapFileType::Map);
  tests_support::ScopedFile map2(mapsDir, platform::CountryFile(it->second[1]), MapFileType::Map);
  ResizeToRemote(map1, storage, it->second[0]);
  ResizeToRemote(map2, storage, it->second[1]);
  ScanTerrain(storage);
  storage.RegisterAllLocalMaps();
  Storage::UpdateInfo updateInfo;
  TEST(storage.GetUpdateInfo(storage.GetRootId(), updateInfo), ());
  // The fake maps are current, but both regions have their terrain to fetch: the
  // update badges must not read "nothing to update" on a terrain-only refresh.
  TEST_EQUAL(updateInfo.m_numberOfMwmFilesToUpdate, 2, ());
  // Counted once for all the co-owners, and nothing older on disk to be replaced.
  TEST_EQUAL(updateInfo.m_totalDownloadSizeInBytes, block.m_size, (it->second));
  TEST_EQUAL(updateInfo.m_maxFileSizeInBytes, block.m_size, ());
  TEST_EQUAL(updateInfo.m_sizeDifference, static_cast<int64_t>(block.m_size), ());

  // Scoped to the subtree: the co-owner leaf carries the block, a not-downloaded
  // region must not carry another region's terrain.
  Storage::UpdateInfo leafInfo;
  TEST(storage.GetUpdateInfo(it->second.front(), leafInfo), ());
  TEST_EQUAL(leafInfo.m_totalDownloadSizeInBytes, block.m_size, (it->second.front()));
  TEST_EQUAL(leafInfo.m_numberOfMwmFilesToUpdate, 1, (it->second.front()));
  Storage::UpdateInfo otherInfo;
  TEST(storage.GetUpdateInfo("Madagascar", otherInfo), ());
  TEST_EQUAL(otherInfo.m_totalDownloadSizeInBytes, 0, ());
  TEST_EQUAL(otherInfo.m_numberOfMwmFilesToUpdate, 0, ());

  // An older file still rendering the area is replaced, not added: the download size
  // stays, the disk does not grow.
  tests_support::ScopedDir terrainDir(TERRAIN_DIR);
  tests_support::ScopedDir versionDir(terrainDir, strings::to_string(block.m_version - 1));
  tests_support::ScopedFile blockFile(base::JoinPath(versionDir.GetRelativePath(), block.m_id + TERRAIN_FILE_EXT),
                                      "twm");
  storage.OnTerrainScanned({MakeTerrainFile(block, block.m_version - 1)});
  Storage::UpdateInfo replacingInfo;
  TEST(storage.GetUpdateInfo(storage.GetRootId(), replacingInfo), ());
  TEST_EQUAL(replacingInfo.m_totalDownloadSizeInBytes, block.m_size, ());
  TEST_EQUAL(replacingInfo.m_sizeDifference, 0, ());
}

UNIT_TEST(Storage_TerrainNodeAttrsFusion)
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
  auto const it =
      std::find_if(coverage.begin(), coverage.end(), [](auto const & entry) { return entry.second.size() == 1; });
  TEST(it != coverage.end(), ());
  CountryId const & region = it->first;
  auto const & block = blocks[it->second.front()];

  // A downloaded map with its one covering block missing.
  Storage storage;
  tests_support::ScopedDir mapsDir(strings::to_string(storage.GetCurrentDataVersion()));
  tests_support::ScopedFile map(mapsDir, platform::CountryFile(region), MapFileType::Map);
  ResizeToRemote(map, storage, region);
  storage.RegisterAllLocalMaps();
  ScanTerrain(storage);

  // The region size is the map plus its terrain coverage; the missing terrain of a
  // map-complete region reads "update available".
  uint64_t const mapSize = storage.GetCountryFile(region).GetRemoteSize();
  NodeAttrs attrs;
  storage.GetNodeAttrs(region, attrs);
  TEST_EQUAL(attrs.m_status, NodeStatus::OnDiskOutOfDate, (region));
  TEST_EQUAL(attrs.m_mwmSize, mapSize + block.m_size, (region));

  // A not-downloaded region advertises its full fused size too (the user decides by
  // the real download cost), but its status stays the map one.
  CountryId const other = "Madagascar";
  uint64_t otherCoverage = 0;
  for (auto const index : coverage[other])
    otherCoverage += blocks[index].m_size;
  NodeAttrs otherAttrs;
  storage.GetNodeAttrs(other, otherAttrs);
  TEST_EQUAL(otherAttrs.m_status, NodeStatus::NotDownloaded, ());
  TEST_EQUAL(otherAttrs.m_mwmSize, storage.GetCountryFile(other).GetRemoteSize() + otherCoverage, ());

  // The complete state: the on-disk terrain keeps the OnDisk status, joins the local
  // size and the progress stays full - the fused size is not only the missing bytes.
  tests_support::ScopedDir terrainDir(TERRAIN_DIR);
  tests_support::ScopedDir versionDir(terrainDir, strings::to_string(block.m_version));
  tests_support::ScopedFile blockFile(
      base::JoinPath(TERRAIN_DIR, strings::to_string(block.m_version), block.m_id + TERRAIN_FILE_EXT), "twm");
  Storage complete;
  complete.RegisterAllLocalMaps();
  ScanTerrain(complete);
  complete.GetNodeAttrs(region, attrs);
  TEST_EQUAL(attrs.m_status, NodeStatus::OnDisk, (region));
  TEST_EQUAL(attrs.m_mwmSize, mapSize + block.m_size, (region));
  TEST_EQUAL(attrs.m_localMwmSize, mapSize + block.m_size, (region));
  // The iOS downloadingSize is the unsigned difference of the two: no wrap allowed.
  TEST_GREATER_OR_EQUAL(attrs.m_downloadingMwmSize, attrs.m_localMwmSize, (region));
  TEST_EQUAL(attrs.m_downloadingProgress.m_bytesDownloaded, attrs.m_downloadingProgress.m_bytesTotal, ());
  TEST_EQUAL(attrs.m_downloadingProgress.m_bytesTotal, static_cast<int64_t>(mapSize + block.m_size), (region));
}

UNIT_TEST(Storage_TerrainNodeAttrsGroupAndInFlight)
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

  // One downloaded Norway leaf needs terrain. Missing sibling maps are not updates;
  // the downloaded leaf's missing terrain still makes the group updatable.
  CountryId const group = "Norway";
  CountryId leaf;
  TaskRunner runner;  // Never run: the enqueued terrain must stay in flight.
  Storage storage;
  ScanTerrain(storage);
  storage.SetDownloaderForTesting(std::make_unique<FakeMapFilesDownloader>(runner));
  storage.ForEachInSubtree(group, [&leaf, &coverage](CountryId const & id, bool groupNode)
  {
    if (!groupNode && leaf.empty() && coverage.count(id) > 0)
      leaf = id;
  });
  TEST(!leaf.empty(), ());

  tests_support::ScopedDir mapsDir(strings::to_string(storage.GetCurrentDataVersion()));
  tests_support::ScopedFile map(mapsDir, platform::CountryFile(leaf), MapFileType::Map);
  ResizeToRemote(map, storage, leaf);
  storage.RegisterAllLocalMaps();

  std::set<uint32_t> groupBlocks;
  uint64_t groupMapSize = 0;
  storage.ForEachInSubtree(group, [&](CountryId const & id, bool groupNode)
  {
    if (groupNode)
      return;
    groupMapSize += storage.GetCountryFile(id).GetRemoteSize();
    if (auto const it = coverage.find(id); it != coverage.end())
      groupBlocks.insert(it->second.begin(), it->second.end());
  });
  uint64_t groupCoverage = 0;
  for (auto const index : groupBlocks)
    groupCoverage += blocks[index].m_size;

  NodeAttrs attrs;
  storage.GetNodeAttrs(group, attrs);
  TEST_EQUAL(attrs.m_status, NodeStatus::OnDiskOutOfDate, ());
  TEST_EQUAL(attrs.m_mwmSize, groupMapSize + groupCoverage, ());

  // Queued terrain of a map-complete leaf lifts it to Downloading; the progress totals
  // the map plus the whole enqueued coverage (never started: the runner is not run).
  storage.DownloadTerrain(leaf);
  uint64_t leafCoverage = 0;
  for (auto const index : coverage[leaf])
    leafCoverage += blocks[index].m_size;
  NodeAttrs leafAttrs;
  storage.GetNodeAttrs(leaf, leafAttrs);
  TEST_EQUAL(leafAttrs.m_status, NodeStatus::Downloading, (leaf));
  TEST_EQUAL(leafAttrs.m_downloadingProgress.m_bytesTotal,
             static_cast<int64_t>(storage.GetCountryFile(leaf).GetRemoteSize() + leafCoverage), (leaf));
  TEST_EQUAL(leafAttrs.m_downloadingProgress.m_bytesDownloaded,
             static_cast<int64_t>(storage.GetCountryFile(leaf).GetRemoteSize()), (leaf));
  storage.GetNodeAttrs(group, attrs);
  TEST_EQUAL(attrs.m_status, NodeStatus::Downloading, ());
}

UNIT_TEST(Storage_TerrainPartialGroupStatus)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  tests_support::ScopedFile countries("terrain_group_status.json", R"({"id": "Countries", "v": 260101,
    "g": [{"id": "Group", "g": [{"id": "West", "s": 2097152}, {"id": "East", "s": 2097152}]}]})");
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, R"({"v": 1,
    "blocks": [{"id": "N45E008", "sx": 3, "sy": 2, "s": 2097152, "h": "aA"}],
    "mwms": {"West": ["N45E008"]}})");
  tests_support::ScopedDirCleanup const mapsDir(base::JoinPath(GetPlatform().WritableDir(), "260101"));
  tests_support::ScopedDirCleanup const terrainDir(base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR));
  tests_support::ScopedFile west("260101/West.mwm", std::string(2 * 1024 * 1024, 'm'));
  Storage storage("terrain_group_status.json");
  storage.Init([](CountryId const &, LocalFilePtr const) {},
               [](CountryId const &, LocalFilePtr const) { return false; });
  storage.RegisterAllLocalMaps();
  storage.OnTerrainScanned({});
  auto downloader = std::make_unique<PendingTerrainDownloader>();
  auto & pending = *downloader;
  storage.SetDownloaderForTesting(std::move(downloader));
  auto const checkStatus = [&](NodeStatus status, NodeErrorCode error = NodeErrorCode::NoError)
  {
    for (auto const & id : {CountryId("West"), CountryId("Group"), storage.GetRootId()})
    {
      NodeAttrs attrs;
      storage.GetNodeAttrs(id, attrs);
      NodeStatuses statuses;
      storage.GetNodeStatuses(id, statuses);
      TEST_EQUAL(attrs.m_status, status, (id));
      TEST_EQUAL(statuses.m_status, status, (id));
      TEST_EQUAL(statuses.m_error, error, (id));
    }
  };
  size_t started = 0;
  storage.SetStartDownloadingCallback([&]
  {
    ++started;
    checkStatus(NodeStatus::Downloading);
  });
  checkStatus(NodeStatus::OnDiskOutOfDate);
  storage.DownloadNode("West");
  pending.Start();
  TEST_EQUAL(started, 1, ());
  checkStatus(NodeStatus::Downloading);
  pending.Fail(downloader::DownloadStatus::FileNotFound);
  checkStatus(NodeStatus::Error, NodeErrorCode::UnknownError);
  storage.CancelTerrain("West");
  checkStatus(NodeStatus::OnDiskOutOfDate);

  // A failed MWM must not hide the other leaf's active terrain download.
  storage.DownloadNode("East");
  storage.DownloadNode("West");
  pending.Start();
  pending.Fail(downloader::DownloadStatus::Failed);
  checkStatus(NodeStatus::Downloading);
  storage.CancelDownloadNode(storage.GetRootId());
}

UNIT_CLASS_TEST(StorageTerrainDownloadTest, SharedTerrainNotifiesInstalledNeighbors)
{
  tests_support::ScopedFile countries("terrain_shared_notifications.json", R"({"id": "Countries", "v": 260101,
    "g": [{"id": "West", "s": 2097152}, {"id": "East", "s": 2097152}]})");
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, R"({"v": 1,
    "blocks": [{"id": "N45E008", "sx": 3, "sy": 2, "s": 2097152, "h": "aA"}],
    "mwms": {"West": ["N45E008"], "East": ["N45E008"]}})");
  tests_support::ScopedDirCleanup const mapsDir(base::JoinPath(GetPlatform().WritableDir(), "260101"));
  tests_support::ScopedDirCleanup const terrainDir(base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR));
  tests_support::ScopedFile west("260101/West.mwm", std::string(2 * 1024 * 1024, 'm'));
  tests_support::ScopedFile east("260101/East.mwm", std::string(2 * 1024 * 1024, 'm'));
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
  auto const slot = storage.Subscribe([&](CountryId const & id)
  { statuses.insert(id); }, [&](CountryId const & id, downloader::Progress const &) { progresses.insert(id); });
  auto const checkNotifications = [&]
  {
    TEST(statuses.contains("West"), (statuses));
    TEST(statuses.contains("East"), (statuses));
    statuses.clear();
  };
  storage.DownloadNode("East");
  pending.Start();
  pending.Progress(1024 * 1024);
  checkNotifications();
  TEST(progresses.contains("West"), (progresses));
  TEST(progresses.contains("East"), (progresses));

  // Notification interest must not acquire ownership of another region's request.
  storage.CancelTerrain("West");
  TEST(storage.IsDownloadInProgress(), ());
  storage.CancelTerrain("East");
  TEST(!storage.IsDownloadInProgress(), ());
  checkNotifications();

  storage.DownloadNode("East");
  pending.Start();
  statuses.clear();
  pending.Complete(TerrainHeaderContent());
  testing::RunEventLoop();
  checkNotifications();
  for (auto const * id : {"West", "East"})
  {
    NodeStatuses status;
    storage.GetNodeStatuses(id, status);
    TEST_EQUAL(status.m_status, NodeStatus::OnDisk, (id));
  }
  storage.Unsubscribe(slot);
}

UNIT_TEST(Storage_DownloadNodeIncludesTerrain)
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
  auto const & region = owner->first;
  auto const & block = blocks[owner->second.front()];

  for (bool const mapOnDisk : {false, true})
  {
    TaskRunner runner;  // Keep the requests queued.
    Storage storage;
    tests_support::ScopedDir mapsDir(strings::to_string(storage.GetCurrentDataVersion()));
    std::unique_ptr<tests_support::ScopedFile> map;
    if (mapOnDisk)
    {
      map = std::make_unique<tests_support::ScopedFile>(mapsDir, platform::CountryFile(region), MapFileType::Map);
      ResizeToRemote(*map, storage, region);
    }
    storage.RegisterAllLocalMaps();
    ScanTerrain(storage);
    auto downloader = std::make_unique<FakeMapFilesDownloader>(runner);
    auto const & queue = downloader->GetQueue();
    storage.SetDownloaderForTesting(std::move(downloader));

    auto const mapSize = storage.GetCountryFile(region).GetRemoteSize();
    TEST_EQUAL(storage.GetDownloadSize({region}), (mapOnDisk ? 0 : mapSize) + block.m_size, (mapOnDisk));
    storage.DownloadNode(region);
    TEST_EQUAL(queue.Count(), mapOnDisk ? 1 : 2, (mapOnDisk));
    TEST_EQUAL(queue.Contains(region), !mapOnDisk, (mapOnDisk));
    TEST_EQUAL(storage.GetTerrainAttrs(region).m_status, Storage::TerrainStatus::Downloading, (mapOnDisk));
    NodeAttrs attrs;
    storage.GetNodeAttrs(region, attrs);
    TEST_EQUAL(attrs.m_status, NodeStatus::Downloading, (mapOnDisk));
    TEST_EQUAL(attrs.m_mwmSize, mapSize + block.m_size, (mapOnDisk));
    TEST_EQUAL(storage.GetDownloadSize({region}), 0, (mapOnDisk));

    // Repeating a download neither re-downloads a current map nor duplicates terrain.
    storage.DownloadNode(region);
    TEST_EQUAL(queue.Count(), mapOnDisk ? 1 : 2, (mapOnDisk));
    storage.CancelDownloadNode(region);
    TEST(queue.IsEmpty(), (mapOnDisk));
    TEST(!storage.IsDownloadInProgress(), (mapOnDisk));
    TEST_EQUAL(storage.GetTerrainAttrs(region).m_status, Storage::TerrainStatus::NotDownloaded, (mapOnDisk));
  }
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

    std::map<CountryId, downloader::Progress> received;
    std::map<CountryId, size_t> notifications;
    size_t statusChanges = 0;
    auto const onProgress = [&](CountryId const & id, downloader::Progress const & progress)
    {
      NodeAttrs attrs;
      storage.GetNodeAttrs(id, attrs);
      TEST_EQUAL(progress.m_bytesDownloaded, attrs.m_downloadingProgress.m_bytesDownloaded, (id, withTerrain));
      TEST_EQUAL(progress.m_bytesTotal, attrs.m_downloadingProgress.m_bytesTotal, (id, withTerrain));
      received[id] = progress;
      ++notifications[id];
    };
    auto const slot = storage.Subscribe([&](CountryId const &) { ++statusChanges; }, onProgress);
    auto const checkProgress = [&](CountryId const & id, uint64_t downloaded, uint64_t total)
    {
      auto const it = received.find(id);
      TEST(it != received.end(), (id, withTerrain));
      TEST_EQUAL(it->second.m_bytesDownloaded, static_cast<int64_t>(downloaded), (id, withTerrain));
      TEST_EQUAL(it->second.m_bytesTotal, static_cast<int64_t>(total), (id, withTerrain));
    };

    storage.DownloadNode("West");
    storage.DownloadNode("East");
    auto const westTotal = (withTerrain ? 10 : 2) * kMegabyte;
    auto const groupTotal = (withTerrain ? 12 : 4) * kMegabyte;
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
    if (withTerrain)
    {
      // A shared block contributes once to the group and to both leaf callbacks,
      // including East, whose map is still waiting behind the terrain in the queue.
      checkProgress("West", 3 * kMegabyte, westTotal);
      checkProgress("East", kMegabyte, 6 * kMegabyte);
    }
    else
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
    storage.GetNodeAttrs(withTerrain ? "West" : "East", attrs);
    TEST_EQUAL(attrs.m_downloadingProgress.m_bytesDownloaded, static_cast<int64_t>((withTerrain ? 4 : 2) * kMegabyte),
               (withTerrain));
    storage.CancelDownloadNode("Group");
  }
}

UNIT_CLASS_TEST(StorageTerrainDownloadTest, GroupProgressCountsOnlyCurrentBatch)
{
  uint64_t constexpr kMapSize = 2 * 1024 * 1024;
  auto const firstContent = TerrainHeaderContent();
  auto const secondContent = TerrainHeaderContent({11, 45, 3, 2});
  auto const firstSize = firstContent.size();
  auto const secondSize = secondContent.size();
  tests_support::ScopedFile countries("terrain_progress_batch.json", R"({"id": "Countries", "v": 260101,
    "g": [{"id": "Group", "g": [{"id": "West", "s": 2097152}, {"id": "Installed", "s": 2097152}]},
          {"id": "Outside", "s": 2097152}]})");
  tests_support::ScopedFile grid(TERRAIN_GRID_FILE, R"({"v": 1, "blocks": [{"id": "N45E008", "sx": 3, "sy": 2, "s": )" +
                                                        strings::to_string(firstSize) + R"(, "h": "aA"},
      {"id": "N45E011", "sx": 3, "sy": 2, "s": )" + strings::to_string(secondSize) +
                                                        R"(, "h": "aA"},
      {"id": "N45E014", "sx": 3, "sy": 2, "s": 2097152, "h": "aA"},
      {"id": "N45E017", "sx": 3, "sy": 2, "s": 2097152, "h": "aA"}],
    "mwms": {"West": ["N45E008", "N45E011"], "Installed": ["N45E014"], "Outside": ["N45E017"]}})");

  for (bool const mapOnDisk : {false, true})
  {
    tests_support::ScopedDirCleanup const mapsDir(base::JoinPath(GetPlatform().WritableDir(), "260101"));
    tests_support::ScopedDirCleanup const terrainDir(base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR));
    tests_support::ScopedFile installedMap("260101/Installed.mwm", std::string(kMapSize, 'm'));
    tests_support::ScopedFile outsideMap("260101/Outside.mwm", std::string(kMapSize, 'm'));
    std::unique_ptr<tests_support::ScopedFile> westMap;
    if (mapOnDisk)
      westMap = std::make_unique<tests_support::ScopedFile>("260101/West.mwm", std::string(kMapSize, 'm'));
    tests_support::ScopedDirCleanup const versionDir(base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR, "1"));
    tests_support::ScopedFile installedTerrain("terrain/1/N45E014.twm", "terrain");
    tests_support::ScopedFile outsideTerrain("terrain/1/N45E017.twm", "terrain");
    Storage storage("terrain_progress_batch.json");
    storage.Init([](CountryId const &, LocalFilePtr const) {},
                 [](CountryId const &, LocalFilePtr const) { return false; });
    storage.SetEnabledIntegrityValidationForTesting(false);
    storage.RegisterAllLocalMaps();
    storage.OnTerrainScanned(
        {{"N45E014", 1, installedTerrain.GetFullPath(), terrain::GridBlock{14, 45, 3, 2}.GetRectMercator()},
         {"N45E017", 1, outsideTerrain.GetFullPath(), terrain::GridBlock{17, 45, 3, 2}.GetRectMercator()}});
    storage.SetTerrainCallbacks([](auto const &)
    {
      testing::StopEventLoop();
      return true;
    }, {});
    auto downloader = std::make_unique<PendingTerrainDownloader>();
    auto & pending = *downloader;
    storage.SetDownloaderForTesting(std::move(downloader));
    auto const checkProgress = [&](CountryId const & id, uint64_t downloaded, uint64_t total)
    {
      NodeAttrs attrs;
      storage.GetNodeAttrs(id, attrs);
      TEST_EQUAL(attrs.m_downloadingProgress.m_bytesDownloaded, static_cast<int64_t>(downloaded), (id, mapOnDisk));
      TEST_EQUAL(attrs.m_downloadingProgress.m_bytesTotal, static_cast<int64_t>(total), (id, mapOnDisk));
    };
    std::map<CountryId, downloader::Progress> received;
    auto const slot = storage.Subscribe([](auto const &) {},
                                        [&](auto const & id, auto const & progress) { received[id] = progress; });
    storage.DownloadNode("West");
    uint64_t batchMapSize = mapOnDisk ? 0 : kMapSize;
    if (!mapOnDisk)
    {
      pending.Start();
      pending.Progress(kMapSize / 2);
      for (auto const & id : {CountryId("Group"), storage.GetRootId()})
      {
        TEST_EQUAL(received.at(id).m_bytesDownloaded, static_cast<int64_t>(kMapSize / 2), (id));
        TEST_EQUAL(received.at(id).m_bytesTotal, static_cast<int64_t>(kMapSize + firstSize + secondSize), (id));
      }
      pending.Complete(std::string(kMapSize, 'm'));
    }
    // Publishing the last MWM must not add unrelated installed maps or terrain.
    checkProgress("Group", batchMapSize, batchMapSize + firstSize + secondSize);
    checkProgress(storage.GetRootId(), batchMapSize, batchMapSize + firstSize + secondSize);
    checkProgress("West", kMapSize, kMapSize + firstSize + secondSize);
    auto const overall = storage.GetOverallProgress({"West"});
    TEST_EQUAL(overall.m_bytesDownloaded, static_cast<int64_t>(kMapSize), (mapOnDisk));
    TEST_EQUAL(overall.m_bytesTotal, static_cast<int64_t>(kMapSize + firstSize + secondSize), (mapOnDisk));

    pending.Start();
    pending.Complete(firstContent);
    testing::RunEventLoop();
    // Keep completed terrain in this batch while its sibling is still pending.
    checkProgress("Group", batchMapSize + firstSize, batchMapSize + firstSize + secondSize);
    checkProgress(storage.GetRootId(), batchMapSize + firstSize, batchMapSize + firstSize + secondSize);
    checkProgress("West", kMapSize + firstSize, kMapSize + firstSize + secondSize);
    if (mapOnDisk)
    {
      storage.CancelDownloadNode("West");
      TEST(!storage.IsDownloadInProgress(), ());
      storage.DownloadNode("West");
      // A new batch excludes the previous batch's completed block, even after cancellation.
      checkProgress("Group", 0, secondSize);
      checkProgress(storage.GetRootId(), 0, secondSize);
      checkProgress("West", kMapSize + firstSize, kMapSize + firstSize + secondSize);
    }
    pending.Start();
    pending.Complete(secondContent);
    testing::RunEventLoop();
    TEST(!storage.IsDownloadInProgress(), (mapOnDisk));
    // Idle, fully installed groups retain the complete on-disk snapshot.
    auto const groupSize = 3 * kMapSize + firstSize + secondSize;
    checkProgress("Group", groupSize, groupSize);
    auto const rootSize = 5 * kMapSize + firstSize + secondSize;
    checkProgress(storage.GetRootId(), rootSize, rootSize);
    storage.Unsubscribe(slot);
  }
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
  // fused status of the map-complete region is Error, with the matching error code.
  auto const run = [&](CountryId const & retryId, downloader::DownloadStatus status, size_t expectedRetryCalls,
                       NodeErrorCode expectedError)
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
    TEST_EQUAL(attrs.m_error, expectedError, (status));
    TEST(storage.CheckFailedCountries({owner->first}), (status));
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
    storage.CancelDownloadNode(owner->first);
  };

  for (auto const & retryId : {owner->first, mapInfo.GetRootId()})
  {
    run(retryId, downloader::DownloadStatus::FileNotFound, 0, NodeErrorCode::UnknownError);
    run(retryId, downloader::DownloadStatus::Failed, 1, NodeErrorCode::NoInetConnection);
  }
}

UNIT_TEST(Storage_TerrainDeleteProtectsQueuedRegion)
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

  // Two regions sharing a block, the first one with a globally exclusive block too
  // (cf. Storage_TerrainRefcountDelete).
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

  // All the blocks of both regions on disk.
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

  // Neither map is downloaded; regionB's map gets QUEUED and never lands (the runner
  // is not run). Its terrain is on disk already, so its own DownloadTerrain records no
  // interest: only the queued map protects the shared block for it
  // (see GetWantedTerrainBlocks).
  TaskRunner runner;
  Storage storage;
  storage.SetDownloaderForTesting(std::make_unique<FakeMapFilesDownloader>(runner));
  ScanTerrain(storage);
  std::vector<TerrainId> deleted;
  storage.SetTerrainCallbacks({}, [&](std::vector<TerrainId> const & ids) { deleted = ids; });
  storage.DownloadNode(regionB);

  auto const contains = [&deleted](TerrainId const & id)
  { return std::find(deleted.begin(), deleted.end(), id) != deleted.end(); };
  storage.DeleteTerrain(regionA);
  TEST(contains(blocks[exclusiveBlock].m_id), ());
  TEST(!contains(blocks[sharedBlock].m_id), ());

  // Cancelled: nobody wants the shared block any more.
  storage.CancelDownloadNode(regionB);
  deleted.clear();
  storage.DeleteTerrain(regionA);
  TEST(contains(blocks[sharedBlock].m_id), ());
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
  storage.CancelDownloadNode("Madagascar");
  storage.DownloadTerrain("Madagascar");
  TEST_EQUAL(pending.m_requests, 2, ());
  GetPlatform().RunTask(Platform::Thread::Gui, [] { testing::StopEventLoop(); });
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
  storage.CancelDownloadNode("Madagascar");
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
      terminalAtNotification = policy.IsAutoRetryDownloadFailed();
      if (failure != 0)
        testing::StopEventLoop();
    }, [](CountryId const &, downloader::Progress const &) {});
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
    storage.CancelDownloadNode("Madagascar");
    TEST(!policy.IsAutoRetryDownloadFailed(), (failure));
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

  storage.RetryDownloadNode("Madagascar");
  TEST_EQUAL(pending.m_requests, 2, ());
  TEST_EQUAL(storage.GetTerrainAttrs("Madagascar").m_status, Storage::TerrainStatus::Downloading, ());
  storage.CancelDownloadNode("Madagascar");
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
  storage.CancelDownloadNode("Madagascar");
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
}  // namespace storage_terrain_tests
