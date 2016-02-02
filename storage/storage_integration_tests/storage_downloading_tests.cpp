#include "testing/testing.hpp"

#include "storage/storage.hpp"

#include "platform/local_country_file_utils.hpp"
#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/write_dir_changer.hpp"

#include "coding/file_name_utils.hpp"

#include "base/scope_guard.hpp"
#include "base/string_utils.hpp"
#include "base/thread.hpp"

#include "std/exception.hpp"
#include "std/string.hpp"

using namespace platform;
using namespace storage;

namespace
{

string const kCountryId = "Angola";

string const kTestWebServer = "http://new-search.mapswithme.com/";

string const kMapTestDir = "map-tests";

void Update(LocalCountryFile const & localCountryFile)
{
  TEST_EQUAL(localCountryFile.GetCountryName(), kCountryId, ());
}

class InterruptException : public exception {};

} // namespace

UNIT_TEST(SmallMwms_InterruptDownloadResumeDownload_Test)
{
  WritableDirChanger writableDirChanger(kMapTestDir);

  // Start download but interrupt it
  try
  {
    Storage storage(COUNTRIES_MIGRATE_FILE);
    TEST(version::IsSingleMwm(storage.GetCurrentDataVersion()), ());

    auto onProgressFn = [](TCountryId const & countryId, LocalAndRemoteSizeT const & mapSize)
    {
      // Interrupt download
      throw InterruptException();
    };

    auto onChangeCountryFn = [&](TCountryId const & countryId)
    {
      if (!storage.IsDownloadInProgress())
        testing::StopEventLoop();
    };

    storage.Init(Update);
    storage.RegisterAllLocalMaps();
    storage.Subscribe(onChangeCountryFn, onProgressFn);
    storage.SetDownloadingUrlsForTesting({kTestWebServer});

    storage.DownloadNode(kCountryId);
    testing::RunEventLoop();

    TEST(false, ()); // If code reaches this point, test fails
  }
  catch (InterruptException const &)
  {}

  // Continue download

  Storage storage(COUNTRIES_MIGRATE_FILE);

  auto onProgressFn = [](TCountryId const & countryId, LocalAndRemoteSizeT const & mapSize) {};
  auto onChangeCountryFn = [&](TCountryId const & countryId)
  {
    if (!storage.IsDownloadInProgress())
      testing::StopEventLoop();
  };

  storage.Init(Update);
  storage.RegisterAllLocalMaps();
  storage.RestoreDownloadQueue();
  storage.Subscribe(onChangeCountryFn, onProgressFn);
  storage.SetDownloadingUrlsForTesting({kTestWebServer});

  TEST(storage.IsDownloadInProgress(), ());

  NodeAttrs attrs;
  storage.GetNodeAttrs(kCountryId, attrs);
  TEST_EQUAL(TStatus::EDownloading, attrs.m_status, ());

  storage.DownloadNode(kCountryId);
  testing::RunEventLoop();

  storage.GetNodeAttrs(kCountryId, attrs);
  TEST_EQUAL(TStatus::EOnDisk, attrs.m_status, ());
}
