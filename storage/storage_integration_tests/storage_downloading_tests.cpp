#include "testing/testing.hpp"

#include "storage/storage_integration_tests/test_defines.hpp"

#include "storage/storage.hpp"

#include "platform/local_country_file_utils.hpp"
#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/writable_dir_changer.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/file_writer.hpp"
#include "coding/sha1.hpp"

#include "base/scope_guard.hpp"
#include "base/string_utils.hpp"
#include "base/thread.hpp"

#include <cstdlib>
#include <functional>
#include <exception>
#include <string>

using namespace platform;
using namespace storage;
using namespace std;
using namespace std::placeholders;

// Uncomment to enable the test that requires network and downloads an mwm several times.
//#define TEST_INTEGRITY
#ifndef TEST_INTEGRITY_ITERATIONS
#define TEST_INTEGRITY_ITERATIONS 5
#endif

namespace
{
using Runner = Platform::ThreadRunner;

string const kCountryId = "Angola";

class InterruptException : public exception {};

void Update(TCountryId const &, storage::TLocalFilePtr const localCountryFile)
{
  TEST_EQUAL(localCountryFile->GetCountryName(), kCountryId, ());
}

void ChangeCountry(Storage & storage, TCountryId const & countryId)
{
  TEST_EQUAL(countryId, kCountryId, ());

  if (!storage.IsDownloadInProgress())
    testing::StopEventLoop();
}

void InitStorage(Storage & storage, Storage::TProgressFunction const & onProgressFn)
{
  storage.Init(Update, [](TCountryId const &, storage::TLocalFilePtr const){return false;});
  storage.RegisterAllLocalMaps(false /* enableDiffs */);
  storage.Subscribe(bind(&ChangeCountry, ref(storage), _1), onProgressFn);
  storage.SetDownloadingUrlsForTesting({kTestWebServer});
}

} // namespace

UNIT_TEST(SmallMwms_ReDownloadExistedMWMIgnored_Test)
{
  WritableDirChanger writableDirChanger(kMapTestDir);
  Storage storage(COUNTRIES_FILE);
  TEST(version::IsSingleMwm(storage.GetCurrentDataVersion()), ());

  InitStorage(storage, [](TCountryId const & countryId, TLocalAndRemoteSize const & mapSize){});
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
    Storage storage(COUNTRIES_FILE);
    TEST(version::IsSingleMwm(storage.GetCurrentDataVersion()), ());

    auto onProgressFn = [](TCountryId const & countryId, TLocalAndRemoteSize const & mapSize)
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
  }

  // Continue download
  {
    Storage storage(COUNTRIES_FILE);

    auto onProgressFn = [](TCountryId const & countryId, TLocalAndRemoteSize const & mapSize)
    {
      TEST_EQUAL(countryId, kCountryId, ());
    };

    InitStorage(storage, onProgressFn);

    TEST(storage.IsDownloadInProgress(), ());

    NodeAttrs attrs;
    storage.GetNodeAttrs(kCountryId, attrs);
    TEST_EQUAL(NodeStatus::Downloading, attrs.m_status, ());

    storage.DownloadNode(kCountryId);
    testing::RunEventLoop();

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
    TEST(version::IsSingleMwm(storage.GetCurrentDataVersion()), ());

    InitStorage(storage, [](TCountryId const & countryId, TLocalAndRemoteSize const & mapSize){});
    TEST(!storage.IsDownloadInProgress(), ());

    storage.DownloadNode(kCountryId);
    TEST(storage.IsDownloadInProgress(), ());
    testing::RunEventLoop();

    auto localFile = storage.GetLatestLocalFile(kCountryId);
    mapPath = localFile->GetPath(MapOptions::Map);
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
      TEST(version::IsSingleMwm(storage.GetCurrentDataVersion()), ());

      auto onProgressFn = [i, j](TCountryId const & countryId,
                                 TLocalAndRemoteSize const & mapSize)
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
      TEST(version::IsSingleMwm(storage.GetCurrentDataVersion()), ());

      InitStorage(storage, [](TCountryId const & countryId, TLocalAndRemoteSize const & mapSize){});
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
