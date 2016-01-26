#include "testing/testing.hpp"

#include "map/framework.hpp"

#include "write_dir_changer.hpp"

#include <QtCore/QCoreApplication>

using namespace platform;

UNIT_TEST(StorageTest_FastMigrate)
{
  // Set clear state.
  string const kMapTestDir = "map-tests";
  WritableDirChanger writableDirChanger(kMapTestDir);

  Framework f;
  auto & s = f.Storage();

  uint32_t version;
  Settings::Get("LastMigration", version);

  TEST_GREATER_OR_EQUAL(s.GetCurrentDataVersion(), version, ());
  Settings::Clear();
}

UNIT_TEST(StorageTests_Migrate)
{
  string const kMapTestDir = "map-tests";
  WritableDirChanger writableDirChanger(kMapTestDir);

  Settings::Set("DisableFastMigrate", true);

  Framework f;
  auto & s = f.Storage();

  auto cleanup = [&]()
  {
    s.DeleteAllLocalMaps();
    Settings::Clear();
  };
  MY_SCOPE_GUARD(cleanupAtExit, cleanup);

  vector<storage::TIndex> const kOldCountries = {s.FindIndexByFile("Estonia")};

  auto stateChanged = [&](storage::TIndex const & id)
  {
    if (!f.Storage().IsDownloadInProgress())
    {
      LOG_SHORT(LINFO, ("All downloaded. Check consistency."));
      QCoreApplication::exit();
    }
  };

  auto progressChanged = [&](storage::TIndex const & id, storage::LocalAndRemoteSizeT const & sz)
  {
    LOG_SHORT(LINFO, (f.GetCountryName(id), "downloading progress:", sz));
  };

  s.Subscribe(stateChanged, progressChanged);

  for (auto const & countryId : kOldCountries)
    f.GetCountryTree().GetActiveMapLayout().DownloadMap(countryId, MapOptions::MapWithCarRouting);

  // Wait for completion of downloading.
  QCoreApplication::exec();

  TEST_EQUAL(s.GetDownloadedFilesCount(), kOldCountries.size(), ());
  for (auto const & countryId : kOldCountries)
    TEST_EQUAL(storage::TStatus::EOnDisk, s.CountryStatusEx(countryId), (countryId));

  f.Migrate();

  vector<storage::TIndex> const kNewCountries = {s.FindIndexByFile("Estonia_East"),
                                                 s.FindIndexByFile("Estonia_West")};

  for (auto const & countryId : kNewCountries)
    f.GetCountryTree().GetActiveMapLayout().DownloadMap(countryId, MapOptions::Map);

  // Wait for completion of downloading.
  QCoreApplication::exec();

  TEST_EQUAL(s.GetDownloadedFilesCount(), kNewCountries.size(), ());
  for (auto const & countryId : kNewCountries)
    TEST_EQUAL(storage::TStatus::EOnDisk, s.CountryStatusEx(countryId), (countryId));
}
