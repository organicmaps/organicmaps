#include "testing/testing.hpp"

#include "storage/storage_integration_tests/test_defines.hpp"

#include "storage/storage.hpp"

#include "platform/platform.hpp"

#include "coding/blake3.hpp"
#include "coding/file_writer.hpp"

#include "base/scope_guard.hpp"

#include "defines.hpp"

#include <functional>
#include <string>

namespace storage_downloading_tests
{
using namespace platform;
using namespace storage;
using namespace std;
using namespace std::placeholders;

// Uncomment to enable the test that downloads an mwm over and over again from kTestWebServer:
// 5 iterations x 10 interrupted downloads plus a full re-download each, roughly 10 minutes and
// 90 MB. Resuming itself is covered offline by storage_tests/storage_download_tests.cpp.
// #define TEST_INTEGRITY
#ifndef TEST_INTEGRITY_ITERATIONS
#define TEST_INTEGRITY_ITERATIONS 5
#endif

namespace
{
string const kCountryId = "Trinidad and Tobago";

void Update(CountryId const &, storage::LocalFilePtr const localCountryFile)
{
  TEST_EQUAL(localCountryFile->GetCountryName(), kCountryId, ());
}

void ChangeCountry(Storage & storage, CountryId const & countryId)
{
  TEST_EQUAL(countryId, kCountryId, ());

  if (!storage.IsDownloadInProgress())
    testing::StopEventLoop();
}

void InitStorage(Storage & storage, Storage::ProgressFunction const & onProgressFn)
{
  storage.Init(Update, [](CountryId const &, storage::LocalFilePtr const) { return false; });
  storage.RegisterAllLocalMaps();
  storage.Subscribe(bind(&ChangeCountry, ref(storage), _1), onProgressFn);
  storage.SetDownloadingServersForTesting({kTestWebServer});
}

}  // namespace

UNIT_CLASS_TEST(StorageTest, SmallMwms_ReDownloadExistedMWMIgnored_Test)
{
  Storage storage;

  InitStorage(storage, [](CountryId const &, downloader::Progress const &) {});
  TEST(!storage.IsDownloadInProgress(), ());

  storage.DownloadNode(kCountryId);
  TEST(storage.IsDownloadInProgress(), ());
  testing::RunEventLoop();

  TEST(!storage.IsDownloadInProgress(), ());
  storage.DownloadNode(kCountryId);
  TEST(!storage.IsDownloadInProgress(), ());
}

UNIT_CLASS_TEST(StorageTest, SmallMwms_InterruptDownloadResumeDownload_Test)
{
  // Start download but interrupt it
  {
    Storage storage;

    auto const onProgressFn = [](CountryId const & countryId, downloader::Progress const & /* progress */)
    {
      TEST_EQUAL(countryId, kCountryId, ());
      // Interrupt download
      testing::StopEventLoop();
    };

    InitStorage(storage, onProgressFn);

    TEST(!storage.IsDownloadInProgress(), ());

    storage.DownloadNode(kCountryId);
    testing::RunEventLoop();

    TEST(storage.IsDownloadInProgress(), ());

    NodeAttrs attrs;
    storage.GetNodeAttrs(kCountryId, attrs);
    TEST_EQUAL(NodeStatus::Downloading, attrs.m_status, ());
  }

  // Continue download
  {
    Storage storage;

    bool onProgressIsCalled = false;
    NodeAttrs onProgressAttrs;
    auto const onProgressFn = [&](CountryId const & countryId, downloader::Progress const & /* progress */)
    {
      TEST_EQUAL(countryId, kCountryId, ());

      if (onProgressIsCalled)
        return;

      onProgressIsCalled = true;
      storage.GetNodeAttrs(kCountryId, onProgressAttrs);
      testing::StopEventLoop();
    };

    InitStorage(storage, onProgressFn);
    storage.Init([](CountryId const &, storage::LocalFilePtr const localCountryFile)
    {
      TEST_EQUAL(localCountryFile->GetCountryName(), kCountryId, ());

      testing::StopEventLoop();
    }, [](CountryId const &, storage::LocalFilePtr const) { return false; });

    // Storage doesn't restore the saved queue by itself, it is an explicit application-level
    // startup step (see Framework::LoadMapsSync), so resume the interrupted download manually.
    storage.RestoreDownloadQueue();

    testing::RunEventLoop();

    TEST(storage.IsDownloadInProgress(), ());

    testing::RunEventLoop();

    TEST_EQUAL(NodeStatus::Downloading, onProgressAttrs.m_status, ());

    NodeAttrs attrs;
    storage.GetNodeAttrs(kCountryId, attrs);
    TEST_EQUAL(NodeStatus::OnDisk, attrs.m_status, ());
  }
}

#ifdef TEST_INTEGRITY
UNIT_CLASS_TEST(StorageTest, DownloadIntegrity_Test)
{
  string mapPath;
  coding::Blake3::Hash mapHash;

  // Wipes the registered map and any stranded .ready file. A round whose interruption came too
  // late completes the transfer, and .ready is renamed to .mwm only by the Gui continuation of
  // the integrity check, which cannot run once the event loop has exited. Leaving it behind
  // would make the next DownloadNode complete synchronously in Storage::DownloadCountry, and
  // the RunEventLoop() below would then wait forever.
  auto const deleteMapFiles = [&mapPath]
  {
    FileWriter::DeleteFileX(mapPath);
    FileWriter::DeleteFileX(mapPath + READY_FILE_EXTENSION);
  };

  {
    SCOPE_GUARD(deleteTestFileGuard, deleteMapFiles);

    Storage storage(COUNTRIES_FILE);

    InitStorage(storage, [](CountryId const &, downloader::Progress const &) {});
    TEST(!storage.IsDownloadInProgress(), ());

    storage.DownloadNode(kCountryId);
    TEST(storage.IsDownloadInProgress(), ());
    testing::RunEventLoop();

    auto localFile = storage.GetLatestLocalFile(kCountryId);
    mapPath = localFile->GetPath(MapFileType::Map);
    mapHash = coding::Blake3::Calculate(mapPath);
  }
  TEST_NOT_EQUAL(mapHash, coding::Blake3::Hash(), ());

  uint32_t constexpr kIterationsCount = TEST_INTEGRITY_ITERATIONS;
  for (uint32_t i = 0; i < kIterationsCount; ++i)
  {
    // Downloading with interruption.
    uint32_t constexpr kInterruptionsCount = 10;
    for (uint32_t j = 0; j < kInterruptionsCount; ++j)
    {
      SCOPE_GUARD(deleteTestFileGuard, deleteMapFiles);

      Storage storage(COUNTRIES_FILE);

      auto onProgressFn = [i, j](CountryId const & countryId, downloader::Progress const & progress)
      {
        TEST_EQUAL(countryId, kCountryId, ());
        auto const fraction = static_cast<double>(progress.m_bytesDownloaded) / progress.m_bytesTotal;
        auto const interruptionFraction =
            0.1 + 0.75 * static_cast<double>((i + j) % kInterruptionsCount) / kInterruptionsCount;
        if (fraction > interruptionFraction)
          testing::StopEventLoop();
      };

      InitStorage(storage, onProgressFn);
      storage.DownloadNode(kCountryId);
      testing::RunEventLoop();
      // StopEventLoop only unwinds the loop, it does not cancel the transfer, so the progress
      // callback that trips the threshold can be the one that also completes the file. The
      // scope guard wipes whatever landed on disk and the next round starts over, which on the
      // last round leaves this iteration hashing a fresh download instead of a resumed one.
      if (!storage.IsDownloadInProgress())
        LOG(LINFO, ("Interruption came too late, the map was downloaded fully."));
    }

    // Continue downloading.
    coding::Blake3::Hash newHash;
    {
      // Delete the completed map when done: otherwise the next iteration's Storage registers it
      // and DownloadNode silently no-ops on the up-to-date map, hanging the event loop wait.
      SCOPE_GUARD(deleteTestFileGuard, deleteMapFiles);

      Storage storage(COUNTRIES_FILE);

      InitStorage(storage, [](CountryId const &, downloader::Progress const &) {});
      // Resumes from the partial download left by the interruption rounds (the downloader picks up
      // the .resume file by path), or starts from scratch if the last round completed the map.
      storage.DownloadNode(kCountryId);
      TEST(storage.IsDownloadInProgress(), ());

      NodeAttrs attrs;
      storage.GetNodeAttrs(kCountryId, attrs);
      TEST_EQUAL(NodeStatus::Downloading, attrs.m_status, ());

      testing::RunEventLoop();

      newHash = coding::Blake3::Calculate(mapPath);
    }

    // Check hashes.
    TEST_EQUAL(mapHash, newHash, ());
  }
}
#endif
}  // namespace storage_downloading_tests
