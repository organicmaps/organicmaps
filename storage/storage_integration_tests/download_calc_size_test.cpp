#include "testing/testing.hpp"

#include "storage/storage_integration_tests/test_defines.hpp"

#include "storage/storage.hpp"

#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/writable_dir_changer.hpp"

using namespace storage;

void InitStorage(Storage & storage, Storage::TUpdateCallback const & didDownload,
                 Storage::TProgressFunction const & progress)
{
  TEST(version::IsSingleMwm(storage.GetCurrentDataVersion()), ());
  
  auto const changeCountryFunction = [&](TCountryId const & /* countryId */)
  {
    if (!storage.IsDownloadInProgress())
    {
      // End wait for downloading complete.
      testing::StopEventLoop();
    }
  };
  
  storage.Init(didDownload, [](TCountryId const &, Storage::TLocalFilePtr const){return false;});
  storage.RegisterAllLocalMaps(false /* enableDiffs */);
  storage.Subscribe(changeCountryFunction, progress);
  storage.SetDownloadingUrlsForTesting({kTestWebServer});
}

UNIT_TEST(DownloadingTests_CalcOverallProgress)
{
  WritableDirChanger writableDirChanger(storage::kMapTestDir);
  
  TCountriesVec const kTestCountries = {
    "Angola",
    "Tokelau",
    "New Zealand North_Auckland",
    "New Zealand North_Wellington"
  };
  
  Storage s;
  
  s.SetDownloadingUrlsForTesting({storage::kTestWebServer});
  MapFilesDownloader::TProgress baseProgress = s.GetOverallProgress(kTestCountries);
  
  TEST_EQUAL(baseProgress.first, 0, ());
  TEST_EQUAL(baseProgress.second, 0, ());
  
  for (auto const &country : kTestCountries)
  {
    baseProgress.second += s.CountrySizeInBytes(country, MapOptions::MapWithCarRouting).second;
  }
  
  auto progressChanged = [&s, &kTestCountries, &baseProgress](TCountryId const & id, TLocalAndRemoteSize const & sz)
  {
    MapFilesDownloader::TProgress currentProgress = s.GetOverallProgress(kTestCountries);
    LOG_SHORT(LINFO, (id, "downloading progress:", currentProgress));
    
    TEST_GREATER_OR_EQUAL(currentProgress.first, baseProgress.first, ());
    baseProgress.first = currentProgress.first;
    
    TEST_LESS_OR_EQUAL(currentProgress.first, baseProgress.second, ());
    TEST_EQUAL(currentProgress.second, baseProgress.second, ());
  };
  
  InitStorage(s, [](storage::TCountryId const &, storage::Storage::TLocalFilePtr const){}, progressChanged);

  for (auto const & countryId : kTestCountries)
    s.DownloadNode(countryId);
  
  testing::RunEventLoop();
}
