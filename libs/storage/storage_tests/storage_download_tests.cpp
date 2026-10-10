#include "testing/testing.hpp"

#include "storage/storage_tests/test_map_files_downloader.hpp"

#include "storage/queued_country.hpp"
#include "storage/storage.hpp"

#include "platform/chunks_download_strategy.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/writable_dir_changer.hpp"
#include "platform/settings.hpp"

#include "coding/blake3.hpp"
#include "coding/file_writer.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "defines.hpp"

#include <cstddef>
#include <cstdint>
#include <future>
#include <memory>
#include <string>
#include <utility>

namespace storage_download_tests
{
using namespace platform;
using namespace storage;

std::string const kTestDir = "storage-download-tests";

std::string const kGroup = "Wonderland";
std::string const kWest = "Wonderland_West";
std::string const kEast = "Wonderland_East";
// Listed in the countries JSON with a hash that does not match what the server serves.
std::string const kCorrupted = "Neverland";

// Synthetic stand-ins for map files. tools/python/test_server serves exactly these bytes
// under /unit_tests/maps/<version>/<name>.mwm; here they only produce the size and the hash
// that go into the countries JSON, so both generators must stay identical - a mismatch
// surfaces as a map integrity failure rather than as a server error.
// Sizes are 2.00-2.25 MB, i.e. 4-5 of the downloader's 512 KB chunks.
size_t constexpr kSyntheticBaseSize = 2 * 1024 * 1024;

uint8_t SyntheticSeed(CountryId const & countryId)
{
  uint32_t sum = 0;
  for (unsigned char const c : countryId + DATA_FILE_EXTENSION)
    sum += c;
  return static_cast<uint8_t>(sum % 256);
}

size_t SyntheticSize(CountryId const & countryId)
{
  return kSyntheticBaseSize + 1024 * SyntheticSeed(countryId);
}

std::string SyntheticContent(CountryId const & countryId)
{
  auto const seed = SyntheticSeed(countryId);
  std::string content(SyntheticSize(countryId), '\0');
  for (size_t i = 0; i < content.size(); ++i)
    content[i] = static_cast<char>((i + seed) % 256);
  return content;
}

std::string SyntheticHash(CountryId const & countryId)
{
  auto const content = SyntheticContent(countryId);
  coding::Blake3 hasher;
  hasher.Update(content.data(), content.size());
  return hasher.FinalizeToBase64(coding::Blake3::kMwmHashSizeInBytes);
}

std::string MakeLeaf(CountryId const & countryId, std::string const & hash)
{
  return R"({"id": ")" + countryId + R"(", "s": )" + strings::to_string(SyntheticSize(countryId)) + R"(, "h": ")" +
         hash + R"("})";
}

std::string const & CountriesJson()
{
  static std::string const json = R"({"id": "Countries", "v": )" + strings::to_string(version::FOR_TESTING_MWM1) +
                                  R"(, "g": [{"id": ")" + kGroup + R"(", "g": [)" +
                                  MakeLeaf(kWest, SyntheticHash(kWest)) + ", " + MakeLeaf(kEast, SyntheticHash(kEast)) +
                                  "]}, " + MakeLeaf(kCorrupted, SyntheticHash(kWest)) + "]}";
  return json;
}

std::string DownloadPath(Storage const & storage, CountryId const & countryId)
{
  return GetFileDownloadPath(storage.GetCurrentDataVersion(), countryId, MapFileType::Map);
}

// Terminal per-country notifications distinguish a registered map from an integrity failure,
// including completion of the asynchronous file validation.
void InitStorage(Storage & storage, Storage::UpdateCallback didDownload,
                 Storage::ChangeCountryFunction changeCountry = [](CountryId const &) {},
                 Storage::ProgressFunction progress = [](CountryId const &, downloader::Progress const &) {})
{
  storage.Init(std::move(didDownload), [](CountryId const &, LocalFilePtr const) { return false; });
  storage.RegisterAllLocalMaps();
  storage.Subscribe(std::move(changeCountry), std::move(progress));
}

void TestDownloadedMap(Storage const & storage, CountryId const & countryId)
{
  auto const localFile = storage.GetLatestLocalFile(countryId);
  TEST(localFile, (countryId));
  TEST_EQUAL(coding::Blake3::CalculateMwmBase64(localFile->GetPath(MapFileType::Map)), SyntheticHash(countryId),
             (countryId));
}

class FileThreadBlocker
{
public:
  FileThreadBlocker()
  {
    auto const release = m_release.get_future().share();
    TEST(GetPlatform().RunTask(Platform::Thread::File, [release]() { release.wait(); }).m_isSuccess, ());
  }

  ~FileThreadBlocker() { Release(); }

  void Release()
  {
    if (!m_released)
    {
      m_release.set_value();
      m_released = true;
    }
  }

private:
  std::promise<void> m_release;
  bool m_released = false;
};

void WaitForFileThread()
{
  std::promise<void> fileIdle;
  auto const idle = fileIdle.get_future();
  TEST(GetPlatform().RunTask(Platform::Thread::File, [&fileIdle]() { fileIdle.set_value(); }).m_isSuccess, ());
  idle.wait();
}

void DrainFileAndGui()
{
  // File tasks and GUI posts are FIFO. Posting the marker from File keeps it deferred on Qt.
  bool guiDrained = false;
  auto const posted = GetPlatform().RunTask(Platform::Thread::File, [&guiDrained]()
  {
    GetPlatform().RunTask(Platform::Thread::Gui, [&guiDrained]()
    {
      guiDrained = true;
      testing::StopEventLoop();
    });
  });
  TEST(posted.m_isSuccess, ());
  testing::RunEventLoop();
  TEST(guiDrained, ());
}

void SeedReadyFile(Storage const & storage, CountryId const & countryId = kWest, bool corrupt = false)
{
  TEST(!PrepareDirToDownloadCountry(storage.GetCurrentDataVersion(), {}).empty(), ());
  auto content = SyntheticContent(countryId);
  if (corrupt)
    content[0] ^= 1;
  FileWriter writer(DownloadPath(storage, countryId));
  writer.Write(content.data(), content.size());
}

void QueueValidation(Storage & storage, CountryId const & countryId = kWest)
{
  QueuedCountry country(storage.GetCountryFile(countryId), countryId, MapFileType::Map, storage.GetCurrentDataVersion(),
                        {}, std::make_shared<diffs::DiffsDataSource>());
  country.Subscribe(storage);
  country.OnStartDownloading();
  auto const size = static_cast<int64_t>(SyntheticSize(countryId));
  country.OnDownloadProgress({size, size});
  country.OnDownloadFinished(downloader::DownloadStatus::Completed);
}

class StorageDownloadTest
{
protected:
  // Storage::OnDownloadFinished validates the downloaded file on Platform::Thread::File, and
  // Platform starts with its thread pools shut down. Declared after the dir changer so the
  // pools are joined before the test directory is removed.
  WritableDirChanger const m_writableDirChanger{kTestDir};
  Platform::ThreadRunner m_threadRunner;
};

UNIT_CLASS_TEST(StorageDownloadTest, DownloadNode)
{
  Storage storage(CountriesJson(), std::make_unique<TestMapFilesDownloader>());
  InitStorage(storage, [](CountryId const &, LocalFilePtr const) { testing::StopEventLoop(); });

  TEST(!storage.IsDownloadInProgress(), ());
  storage.DownloadNode(kWest);
  TEST(storage.IsDownloadInProgress(), ());
  testing::RunEventLoop();

  NodeAttrs attrs;
  storage.GetNodeAttrs(kWest, attrs);
  TEST_EQUAL(NodeStatus::OnDisk, attrs.m_status, ());
  TestDownloadedMap(storage, kWest);

  // An up-to-date map is not re-downloaded: Storage::DownloadNode returns on OnDisk.
  storage.DownloadNode(kWest);
  TEST(!storage.IsDownloadInProgress(), ());
}

UNIT_CLASS_TEST(StorageDownloadTest, RestorePartialDownload)
{
  int64_t constexpr kChunkSize = 512 * 1024;
  auto const version = version::FOR_TESTING_MWM1;
  TEST(!PrepareDirToDownloadCountry(version, {}).empty(), ());
  auto const downloadPath = GetFileDownloadPath(version, kWest, MapFileType::Map);
  auto const content = SyntheticContent(kWest);
  // Seed one completed chunk so the resume test does not depend on download timing.
  {
    FileWriter writer(downloadPath + DOWNLOADING_FILE_EXTENSION);
    writer.Write(content.data(), kChunkSize);
  }

  downloader::ChunksDownloadStrategy chunks({"test-server"});
  chunks.InitChunks(content.size(), kChunkSize);
  std::string url;
  std::pair<int64_t, int64_t> range;
  TEST_EQUAL(chunks.NextChunk(url, range), downloader::ChunksDownloadStrategy::ENextChunk, ());
  TEST_EQUAL(range, std::make_pair(int64_t{0}, kChunkSize - 1), ());
  chunks.ChunkFinished(true, range);
  chunks.SaveChunks(content.size(), downloadPath + RESUME_FILE_EXTENSION);
  settings::Set("DownloadQueue", kWest);

  int64_t firstProgress = -1;
  {
    Storage storage(CountriesJson(), std::make_unique<TestMapFilesDownloader>());
    InitStorage(storage, [](CountryId const &, LocalFilePtr const) { testing::StopEventLoop(); },
                [](CountryId const &) {},
                [&firstProgress](CountryId const & countryId, downloader::Progress const & progress)
    {
      if (countryId == kWest && firstProgress < 0)
        firstProgress = progress.m_bytesDownloaded;
    });

    // Storage does not restore the saved queue by itself, it is an explicit application
    // startup step (see Framework::LoadMapsSync).
    storage.RestoreDownloadQueue();
    TEST(storage.IsDownloadInProgress(), ());
    testing::RunEventLoop();

    TEST_GREATER(firstProgress, kChunkSize, ());
    TestDownloadedMap(storage, kWest);
  }

  TEST(!Platform::IsFileExistsByFullPath(downloadPath + DOWNLOADING_FILE_EXTENSION), (downloadPath));
  TEST(!Platform::IsFileExistsByFullPath(downloadPath + RESUME_FILE_EXTENSION), (downloadPath));
}

UNIT_CLASS_TEST(StorageDownloadTest, ValidationCallbackAfterStorageDestruction)
{
  bool didDownload = false;
  {
    Storage storage(CountriesJson(), std::make_unique<TestMapFilesDownloader>());
    InitStorage(storage, [&didDownload](CountryId const &, LocalFilePtr const) { didDownload = true; });

    SeedReadyFile(storage);
    QueueValidation(storage);
    // Queue the GUI completion before destroying its owner, without executing it.
    WaitForFileThread();
  }

  DrainFileAndGui();
  TEST(!didDownload, ());
}

UNIT_CLASS_TEST(StorageDownloadTest, CancelDuringValidation)
{
  bool didDownload = false;
  Storage storage(CountriesJson(), std::make_unique<TestMapFilesDownloader>());
  InitStorage(storage, [&didDownload](CountryId const &, LocalFilePtr const) { didDownload = true; });
  SeedReadyFile(storage);
  FileThreadBlocker blocker;
  QueueValidation(storage);
  storage.CancelDownloadNode(kWest);
  TEST(!storage.IsDownloadInProgress(), ());
  blocker.Release();
  DrainFileAndGui();
  TEST(!didDownload, ());
  TEST_EQUAL(storage.CountryStatusEx(kWest), Status::NotDownloaded, ());
  TEST(!Platform::IsFileExistsByFullPath(DownloadPath(storage, kWest)), ());
}

UNIT_CLASS_TEST(StorageDownloadTest, ClearDuringValidation)
{
  bool didDownload = false;
  Storage storage(CountriesJson(), std::make_unique<TestMapFilesDownloader>());
  InitStorage(storage, [&didDownload](CountryId const &, LocalFilePtr const) { didDownload = true; });
  SeedReadyFile(storage);
  FileThreadBlocker blocker;
  QueueValidation(storage);
  storage.Clear();
  TEST(!storage.IsDownloadInProgress(), ());
  blocker.Release();
  DrainFileAndGui();
  TEST(!didDownload, ());
  TEST(!Platform::IsFileExistsByFullPath(DownloadPath(storage, kWest)), ());
}

UNIT_CLASS_TEST(StorageDownloadTest, RestoreUnvalidatedReadyFile)
{
  base::ScopedLogAbortLevelChanger const ignoreErrors(LCRITICAL);
  FileThreadBlocker blocker;
  {
    Storage storage(CountriesJson(), std::make_unique<TestMapFilesDownloader>());
    InitStorage(storage, [](CountryId const &, LocalFilePtr const) {});
    SeedReadyFile(storage, kWest, true);
    QueueValidation(storage);
  }

  bool didDownload = false;
  Storage storage(CountriesJson(), std::make_unique<TestMapFilesDownloader>());
  InitStorage(storage, [&didDownload](CountryId const &, LocalFilePtr const) { didDownload = true; });
  settings::Set("DownloadQueue", kWest);
  storage.RestoreDownloadQueue();
  TEST(storage.IsDownloadInProgress(), ());
  blocker.Release();
  DrainFileAndGui();
  TEST(!didDownload, ());
  TEST_EQUAL(storage.CountryStatusEx(kWest), Status::DownloadFailed, ());
  TEST(!Platform::IsFileExistsByFullPath(DownloadPath(storage, kWest)), ());
}

UNIT_CLASS_TEST(StorageDownloadTest, ValidationIsDownloadInProgress)
{
  Storage storage(CountriesJson(), std::make_unique<TestMapFilesDownloader>());
  InitStorage(storage, [](CountryId const &, LocalFilePtr const) {});
  SeedReadyFile(storage);
  FileThreadBlocker blocker;
  QueueValidation(storage);
  TEST(storage.IsDownloadInProgress(), ());
  TEST_EQUAL(storage.CountryStatusEx(kWest), Status::Downloading, ());
  blocker.Release();
  DrainFileAndGui();
  TEST(!storage.IsDownloadInProgress(), ());
  TestDownloadedMap(storage, kWest);
}

UNIT_CLASS_TEST(StorageDownloadTest, CancelThenNewValidation)
{
  size_t downloaded = 0;
  Storage storage(CountriesJson(), std::make_unique<TestMapFilesDownloader>());
  InitStorage(storage, [&downloaded](CountryId const &, LocalFilePtr const) { ++downloaded; });
  SeedReadyFile(storage);
  QueueValidation(storage);
  WaitForFileThread();
  storage.CancelDownloadNode(kWest);
  SeedReadyFile(storage);
  QueueValidation(storage);
  DrainFileAndGui();
  TEST_EQUAL(downloaded, 1, ());
  TEST_EQUAL(storage.CountryStatusEx(kWest), Status::OnDisk, ());
  TestDownloadedMap(storage, kWest);
}

UNIT_CLASS_TEST(StorageDownloadTest, DuplicateDownloadDuringValidation)
{
  size_t started = 0;
  size_t downloaded = 0;
  Storage storage(CountriesJson(), std::make_unique<TestMapFilesDownloader>());
  InitStorage(storage, [&downloaded](CountryId const &, LocalFilePtr const) { ++downloaded; });
  storage.SetStartDownloadingCallback([&started]() { ++started; });
  SeedReadyFile(storage);
  FileThreadBlocker blocker;
  QueueValidation(storage);
  storage.DownloadNode(kWest);
  TEST_EQUAL(started, 1, ());
  blocker.Release();
  DrainFileAndGui();
  TEST_EQUAL(downloaded, 1, ());
  TestDownloadedMap(storage, kWest);
}

UNIT_CLASS_TEST(StorageDownloadTest, GroupProgressIncludesValidatedCountry)
{
  Storage storage(CountriesJson(), std::make_unique<TestMapFilesDownloader>());
  InitStorage(storage, [](CountryId const &, LocalFilePtr const) {});
  SeedReadyFile(storage);
  SeedReadyFile(storage, kEast);
  FileThreadBlocker beforeFirstValidation;
  QueueValidation(storage);
  int64_t bytesTotal = 0;
  std::string savedQueue;
  GetPlatform().RunTask(Platform::Thread::File, [&storage, &bytesTotal, &savedQueue]()
  {
    GetPlatform().RunTask(Platform::Thread::Gui, [&storage, &bytesTotal, &savedQueue]()
    {
      NodeAttrs attrs;
      storage.GetNodeAttrs(kGroup, attrs);
      bytesTotal = attrs.m_downloadingProgress.m_bytesTotal;
      settings::TryGet("DownloadQueue", savedQueue);
      testing::StopEventLoop();
    });
  });
  FileThreadBlocker beforeSecondValidation;
  QueueValidation(storage, kEast);
  beforeFirstValidation.Release();
  testing::RunEventLoop();
  beforeSecondValidation.Release();
  DrainFileAndGui();
  TEST_EQUAL(bytesTotal, static_cast<int64_t>(SyntheticSize(kWest) + SyntheticSize(kEast)), ());
  TEST_EQUAL(savedQueue, kEast, ());
}

UNIT_CLASS_TEST(StorageDownloadTest, CancelRestoredFileFromStartCallback)
{
  bool didDownload = false;
  Storage storage(CountriesJson(), std::make_unique<TestMapFilesDownloader>());
  InitStorage(storage, [&didDownload](CountryId const &, LocalFilePtr const) { didDownload = true; });
  SeedReadyFile(storage);
  storage.SetStartDownloadingCallback([&storage]() { storage.CancelDownloadNode(kWest); });
  storage.DownloadNode(kWest);
  DrainFileAndGui();
  TEST(!didDownload, ());
  TEST(!storage.IsDownloadInProgress(), ());
  TEST_EQUAL(storage.CountryStatusEx(kWest), Status::NotDownloaded, ());
}

UNIT_CLASS_TEST(StorageDownloadTest, DownloadWithoutHash)
{
  auto const json = R"({"id": "Countries", "v": )" + strings::to_string(version::FOR_TESTING_MWM1) + R"(, "g": [)" +
                    MakeLeaf(kWest, {}) + "]}";
  bool didDownload = false;
  Storage storage(json, std::make_unique<TestMapFilesDownloader>());
  InitStorage(storage, [&didDownload](CountryId const &, LocalFilePtr const) { didDownload = true; });
  SeedReadyFile(storage);
  storage.DownloadNode(kWest);
  DrainFileAndGui();
  TEST(didDownload, ());
  TestDownloadedMap(storage, kWest);
}

UNIT_CLASS_TEST(StorageDownloadTest, DownloadAndDeleteGroup)
{
  Storage storage(CountriesJson(), std::make_unique<TestMapFilesDownloader>());

  size_t downloaded = 0;
  InitStorage(storage, [&downloaded](CountryId const &, LocalFilePtr const)
  {
    if (++downloaded == 2)
      testing::StopEventLoop();
  });

  storage.DownloadNode(kGroup);
  testing::RunEventLoop();

  NodeAttrs attrs;
  storage.GetNodeAttrs(kGroup, attrs);
  TEST_EQUAL(NodeStatus::OnDisk, attrs.m_status, ());
  for (auto const & leaf : {kWest, kEast})
    TestDownloadedMap(storage, leaf);

  storage.DeleteNode(kGroup);

  storage.GetNodeAttrs(kGroup, attrs);
  TEST_EQUAL(NodeStatus::NotDownloaded, attrs.m_status, ());
  for (auto const & leaf : {kWest, kEast})
  {
    TEST_EQUAL(Status::NotDownloaded, storage.CountryStatusEx(leaf), (leaf));
    TEST(!Platform::IsFileExistsByFullPath(GetFilePath(storage.GetCurrentDataVersion(), {}, leaf, MapFileType::Map)),
         (leaf));
  }
}

UNIT_CLASS_TEST(StorageDownloadTest, FailedIntegrityCheck)
{
  // Rejecting the map is logged as an error, which aborts the test binary by default.
  base::ScopedLogAbortLevelChanger const ignoreErrors(LCRITICAL);

  Storage storage(CountriesJson(), std::make_unique<TestMapFilesDownloader>());
  InitStorage(storage, [](CountryId const &, LocalFilePtr const) { TEST(false, ("Corrupted map was registered")); },
              [&storage](CountryId const & countryId)
  {
    NodeAttrs attrs;
    storage.GetNodeAttrs(countryId, attrs);
    if (attrs.m_status == NodeStatus::Error)
      testing::StopEventLoop();
  });

  storage.DownloadNode(kCorrupted);
  testing::RunEventLoop();

  // The hash is verified only after the transfer completes, so the fully downloaded file has
  // to be dropped instead of being registered as a map.
  TEST(!storage.GetLatestLocalFile(kCorrupted), ());
  TEST(!Platform::IsFileExistsByFullPath(DownloadPath(storage, kCorrupted)), ());
}
}  // namespace storage_download_tests
