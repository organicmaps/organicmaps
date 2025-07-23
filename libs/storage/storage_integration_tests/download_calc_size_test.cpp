#include "testing/testing.hpp"

#include "storage/storage_integration_tests/test_defines.hpp"

#include "storage/storage.hpp"

#include "platform/downloader_defines.hpp"
#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/writable_dir_changer.hpp"

namespace download_calc_size_test
{
using namespace storage;
void InitStorage(Storage & storage, Storage::UpdateCallback const & didDownload,
                 Storage::ProgressFunction const & progress)
{
  auto const changeCountryFunction = [&](CountryId const & /* countryId */)
  {
    if (!storage.IsDownloadInProgress())
    {
      // End wait for downloading complete.
      testing::StopEventLoop();
    }
  };

  storage.Init(didDownload, [](CountryId const &, LocalFilePtr const) { return false; });
  storage.RegisterAllLocalMaps();
  storage.Subscribe(changeCountryFunction, progress);
  storage.SetDownloadingServersForTesting({kTestWebServer});
}

UNIT_TEST(DownloadingTests_CalcOverallProgress)
{
  WritableDirChanger writableDirChanger(storage::kMapTestDir);

  // A bunch of small islands.
  CountriesVec const kTestCountries = {"Kiribati", "Tokelau", "Niue", "Palau", "Pitcairn Islands"};

  Storage s;

  s.SetDownloadingServersForTesting({storage::kTestWebServer});
  auto baseProgress = s.GetOverallProgress(kTestCountries);

  TEST_EQUAL(baseProgress.m_bytesDownloaded, 0, ());
  TEST_EQUAL(baseProgress.m_bytesTotal, 0, ());

  for (auto const & country : kTestCountries)
    baseProgress.m_bytesTotal += s.CountrySizeInBytes(country).second;

  auto progressChanged =
      [&s, &kTestCountries, &baseProgress](CountryId const & id, downloader::Progress const & /* progress */)
  {
    auto const currentProgress = s.GetOverallProgress(kTestCountries);
    LOG_SHORT(LINFO, (id, "downloading progress:", currentProgress));

    TEST_GREATER_OR_EQUAL(currentProgress.m_bytesDownloaded, baseProgress.m_bytesDownloaded, ());
    baseProgress.m_bytesDownloaded = currentProgress.m_bytesDownloaded;

    TEST_LESS_OR_EQUAL(currentProgress.m_bytesDownloaded, baseProgress.m_bytesTotal, ());
    TEST_EQUAL(currentProgress.m_bytesTotal, baseProgress.m_bytesTotal, ());
  };

  InitStorage(s, [](storage::CountryId const &, LocalFilePtr const) {}, progressChanged);

  for (auto const & countryId : kTestCountries)
    s.DownloadNode(countryId);

  testing::RunEventLoop();
}
}  // namespace download_calc_size_test
