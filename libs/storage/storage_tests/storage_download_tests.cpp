#include "testing/testing.hpp"

#include "storage/storage_tests/test_map_files_downloader.hpp"

#include "storage/storage.hpp"

#include "platform/local_country_file_utils.hpp"
#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/writable_dir_changer.hpp"

#include "coding/blake3.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "defines.hpp"

#include <cstddef>
#include <cstdint>
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
// Sizes are 2.00-2.25 MB, i.e. 4-5 of the downloader's 512 KB chunks, so an interrupted
// download always leaves a partial file to resume from.
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

// Storage completes a download in two steps: HttpMapFilesDownloader pops the country from the
// queue, and only then Storage::OnDownloadFinished hands the integrity check to
// Platform::Thread::File. An empty queue is therefore observable while a map is still being
// validated, so the tests below wait for the terminal per-country notifications instead - the
// update callback for a registered map, NodeStatus::Error for a rejected one.
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

UNIT_CLASS_TEST(StorageDownloadTest, ResumeInterruptedDownload)
{
  std::string downloadPath;

  {
    Storage storage(CountriesJson(), std::make_unique<TestMapFilesDownloader>());
    InitStorage(storage, [](CountryId const &, LocalFilePtr const) {}, [](CountryId const &) {},
                [](CountryId const &, downloader::Progress const &) { testing::StopEventLoop(); });

    storage.DownloadNode(kWest);
    testing::RunEventLoop();
    downloadPath = DownloadPath(storage, kWest);
    // Destroying Storage cancels the transfer. HttpMapFilesDownloader passes
    // doCleanOnCancel = false, so the partial file and its resume data stay on disk.
  }

  TEST(Platform::IsFileExistsByFullPath(downloadPath + DOWNLOADING_FILE_EXTENSION), (downloadPath));
  TEST(Platform::IsFileExistsByFullPath(downloadPath + RESUME_FILE_EXTENSION), (downloadPath));

  {
    Storage storage(CountriesJson(), std::make_unique<TestMapFilesDownloader>());
    InitStorage(storage, [](CountryId const &, LocalFilePtr const) { testing::StopEventLoop(); });

    // Storage does not restore the saved queue by itself, it is an explicit application
    // startup step (see Framework::LoadMapsSync).
    storage.RestoreDownloadQueue();
    TEST(storage.IsDownloadInProgress(), ());
    testing::RunEventLoop();

    TestDownloadedMap(storage, kWest);
  }

  TEST(!Platform::IsFileExistsByFullPath(downloadPath + DOWNLOADING_FILE_EXTENSION), (downloadPath));
  TEST(!Platform::IsFileExistsByFullPath(downloadPath + RESUME_FILE_EXTENSION), (downloadPath));
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
