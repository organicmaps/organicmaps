#include "testing/testing.hpp"

#include "storage/storage_integration_tests/test_defines.hpp"

#include "storage/storage.hpp"

#include "platform/local_country_file_utils.hpp"
#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/writable_dir_changer.hpp"

#include "coding/file_writer.hpp"
#include "coding/sha1.hpp"

#include "base/file_name_utils.hpp"
#include "base/scope_guard.hpp"
#include "base/string_utils.hpp"
#include "base/thread.hpp"

#include <cstdlib>
#include <exception>
#include <functional>
#include <string>

using namespace platform;
using namespace storage;
using namespace std;
using namespace std::placeholders;

// Uncomment to enable the test that requires network and downloads an mwm several times.
// #define TEST_INTEGRITY
#ifndef TEST_INTEGRITY_ITERATIONS
#define TEST_INTEGRITY_ITERATIONS 5
#endif

namespace
{
using Runner = Platform::ThreadRunner;

string const kCountryId = "Trinidad and Tobago";

class InterruptException : public exception
{};

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

UNIT_TEST(SmallMwms_ReDownloadExistedMWMIgnored_Test)
{
  WritableDirChanger writableDirChanger(kMapTestDir);
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

UNIT_CLASS_TEST(Runner, SmallMwms_InterruptDownloadResumeDownload_Test)
{
  WritableDirChanger writableDirChanger(kMapTestDir);

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
UNIT_CLASS_TEST(Runner, DownloadIntegrity_Test)
{
  WritableDirChanger writableDirChanger(kMapTestDir);

  string mapPath;
  coding::SHA1::Hash mapHash;
  {
    SCOPE_GUARD(deleteTestFileGuard, bind(&FileWriter::DeleteFileX, ref(mapPath)));

    Storage storage(COUNTRIES_FILE);

    InitStorage(storage, [](CountryId const & countryId, LocalAndRemoteSize const & mapSize) {});
    TEST(!storage.IsDownloadInProgress(), ());

    storage.DownloadNode(kCountryId);
    TEST(storage.IsDownloadInProgress(), ());
    testing::RunEventLoop();

    auto localFile = storage.GetLatestLocalFile(kCountryId);
    mapPath = localFile->GetPath(MapFileType::Map);
    mapHash = coding::SHA1::Calculate(mapPath);
  }
  TEST_NOT_EQUAL(mapHash, coding::SHA1::Hash(), ());

  uint32_t constexpr kIterationsCount = TEST_INTEGRITY_ITERATIONS;
  for (uint32_t i = 0; i < kIterationsCount; ++i)
  {
    // Downloading with interruption.
    uint32_t constexpr kInterruptionsCount = 10;
    for (uint32_t j = 0; j < kInterruptionsCount; ++j)
    {
      SCOPE_GUARD(deleteTestFileGuard, bind(&FileWriter::DeleteFileX, ref(mapPath)));

      Storage storage(COUNTRIES_FILE);

      auto onProgressFn = [i, j](CountryId const & countryId, LocalAndRemoteSize const & mapSize)
      {
        TEST_EQUAL(countryId, kCountryId, ());
        auto progress = static_cast<double>(mapSize.first) / mapSize.second;
        auto interruptionProgress =
            0.1 + 0.75 * static_cast<double>((i + j) % kInterruptionsCount) / kInterruptionsCount;
        if (progress > interruptionProgress)
          testing::StopEventLoop();
      };

      InitStorage(storage, onProgressFn);
      storage.DownloadNode(kCountryId);
      testing::RunEventLoop();
      TEST(storage.IsDownloadInProgress(), ());
    }

    // Continue downloading.
    coding::SHA1::Hash newHash;
    {
      Storage storage(COUNTRIES_FILE);

      InitStorage(storage, [](CountryId const & countryId, LocalAndRemoteSize const & mapSize) {});
      TEST(storage.IsDownloadInProgress(), ());

      NodeAttrs attrs;
      storage.GetNodeAttrs(kCountryId, attrs);
      TEST_EQUAL(NodeStatus::Downloading, attrs.m_status, ());

      storage.DownloadNode(kCountryId);
      TEST(storage.IsDownloadInProgress(), ());
      testing::RunEventLoop();

      newHash = coding::SHA1::Calculate(mapPath);
    }

    // Check hashes.
    TEST_EQUAL(mapHash, newHash, ());
  }
}
#endif
