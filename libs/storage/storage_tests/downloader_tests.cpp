#include "testing/testing.hpp"

#include "storage/map_files_downloader_with_ping.hpp"
#include "storage/storage_tests/test_map_files_downloader.hpp"

#include "platform/local_country_file_utils.hpp"
#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/writable_dir_changer.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "defines.hpp"
#include "private.h"

namespace downloader_tests
{

class DownloaderStub : public storage::MapFilesDownloaderWithPing
{
public:
  using storage::MapFilesDownloader::MakeUrlList;

  void Download(storage::QueuedCountry &&) override {}
};

UNIT_TEST(NormalizeDebugMapDownloadServer)
{
  std::string normalized;

  TEST(storage::NormalizeDebugMapDownloadServer("https://example.com", normalized), ());
  TEST_EQUAL(normalized, "https://example.com/", ());

  TEST(storage::NormalizeDebugMapDownloadServer(" http://localhost:8000/maps ", normalized), ());
  TEST_EQUAL(normalized, "http://localhost:8000/maps/", ());

  TEST(!storage::NormalizeDebugMapDownloadServer("", normalized), ());
  TEST(!storage::NormalizeDebugMapDownloadServer("http:/example.com", normalized), ());
  TEST(!storage::NormalizeDebugMapDownloadServer("https:///example.com", normalized), ());
  TEST(!storage::NormalizeDebugMapDownloadServer("ftp://example.com", normalized), ());
  TEST(!storage::NormalizeDebugMapDownloadServer("https://example.com?token=1", normalized), ());
  TEST(!storage::NormalizeDebugMapDownloadServer("https://exa mple.com", normalized), ());
}

UNIT_TEST(MapFilesDownloader_DebugServerList)
{
  DownloaderStub downloader;
  downloader.SetServersList({"https://example.com/base/"});

  auto urls = downloader.MakeUrlList("maps/1/Germany.mwm");
  TEST_EQUAL(urls.size(), 1, ());
  TEST_EQUAL(urls[0], "https://example.com/base/maps/1/Germany.mwm", ());

  downloader.ResetServersList();
  TEST(downloader.MakeUrlList("maps/1/Germany.mwm").empty(), ());
}

UNIT_TEST(GetMetaConfig)
{
  if (std::string(METASERVER_URL).empty())
    return;

  base::ScopedLogLevelChanger logLevel(base::LDEBUG);
  Platform::ThreadRunner runner;

  auto const metaConfig = DownloaderStub().GetMetaConfig();
  TEST_GREATER(metaConfig.servers.size(), 0, ());
  for (auto const & s : metaConfig.servers)
    LOG(LINFO, (s));
}

class HttpDownloaderTest : public storage::QueuedCountry::Subscriber
{
public:
  ~HttpDownloaderTest() override { m_downloader.Clear(); }

  void OnCountryInQueue(storage::QueuedCountry const &) override {}
  void OnStartDownloading(storage::QueuedCountry const & country) override { m_onStart(country); }
  void OnDownloadProgress(storage::QueuedCountry const &, downloader::Progress const &) override {}
  void OnDownloadFinished(storage::QueuedCountry const &, downloader::DownloadStatus status) override
  {
    TEST_EQUAL(status, downloader::DownloadStatus::Failed, ());
    ++m_failures;
  }

protected:
  void Enqueue(storage::CountryId const & id, int64_t version = version::FOR_TESTING_MWM1)
  {
    TEST(!platform::PrepareDirToDownloadCountry(version, {}).empty(), ());
    storage::QueuedCountry country(platform::CountryFile(id, 1, {}), id, MapFileType::Map, version, {},
                                   std::make_shared<storage::diffs::DiffsDataSource>());
    country.Subscribe(*this);
    m_downloader.DownloadMapFile(std::move(country));
  }

  std::string DownloadingPath(storage::CountryId const & id, int64_t version = version::FOR_TESTING_MWM1) const
  {
    return platform::GetFileDownloadPath(version, id, MapFileType::Map) + DOWNLOADING_FILE_EXTENSION;
  }

  WritableDirChanger const m_dir{"http-downloader-tests", WritableDirChanger::SettingsDirPolicy::UseWritableDir};
  Platform::ThreadRunner m_threads;
  storage::TestMapFilesDownloader m_downloader;
  std::function<void(storage::QueuedCountry const &)> m_onStart;
  size_t m_failures = 0;
};

UNIT_CLASS_TEST(HttpDownloaderTest, CancelFromStartCallback)
{
  for (bool const clear : {false, true})
  {
    m_onStart = [&](storage::QueuedCountry const & country)
    {
      auto const id = country.GetCountryId();
      if (clear)
        m_downloader.Clear();
      else
        m_downloader.Remove(id);
    };
    Enqueue("Wonderland_West");
    TEST(m_downloader.GetQueue().IsEmpty(), (clear));
    TEST(!Platform::IsFileExistsByFullPath(DownloadingPath("Wonderland_West")), (clear));
  }
}

UNIT_CLASS_TEST(HttpDownloaderTest, ReplaceFromStartCallback)
{
  for (bool const failReplacement : {false, true})
  {
    int64_t const newVersion = version::FOR_TESTING_MWM1 + (failReplacement ? 2 : 1);
    size_t starts = 0;
    m_onStart = [&](storage::QueuedCountry const & country)
    {
      if (++starts == 1)
      {
        auto const id = country.GetCountryId();
        m_downloader.Remove(id);
        Enqueue(id, newVersion);
      }
      else if (failReplacement)
      {
        // Force request creation to fail: the old attempt must still stay cancelled.
        TEST_EQUAL(Platform::RmDir(base::GetDirectory(country.GetFileDownloadPath())), Platform::ERR_OK, ());
      }
    };
    Enqueue("Wonderland_West");
    TEST_EQUAL(starts, 2, (failReplacement));
    TEST_EQUAL(m_downloader.GetQueue().Count() + m_failures, 1, (failReplacement));
    TEST(!Platform::IsFileExistsByFullPath(DownloadingPath("Wonderland_West")), (failReplacement));
    TEST_EQUAL(Platform::IsFileExistsByFullPath(DownloadingPath("Wonderland_West", newVersion)), !failReplacement,
               (failReplacement));
    m_downloader.Clear();
  }
}

UNIT_CLASS_TEST(HttpDownloaderTest, AdvanceQueueFromStartCallback)
{
  size_t starts = 0;
  m_onStart = [&](storage::QueuedCountry const & country)
  {
    if (++starts == 1)
    {
      auto const id = country.GetCountryId();
      Enqueue("Wonderland_East");
      m_downloader.Remove(id);
    }
  };
  Enqueue("Wonderland_West");
  TEST_EQUAL(starts, 2, ());
  TEST_EQUAL(m_downloader.GetQueue().Count(), 1, ());
  TEST(m_downloader.GetQueue().Contains("Wonderland_East"), ());
  TEST(!Platform::IsFileExistsByFullPath(DownloadingPath("Wonderland_West")), ());
  TEST(Platform::IsFileExistsByFullPath(DownloadingPath("Wonderland_East")), ());
}

UNIT_CLASS_TEST(HttpDownloaderTest, RemoveQueuedCountryFromStartCallback)
{
  size_t starts = 0;
  m_onStart = [&](storage::QueuedCountry const &)
  {
    if (++starts == 1)
    {
      Enqueue("Wonderland_East");
      m_downloader.Remove("Wonderland_East");
    }
  };
  Enqueue("Wonderland_West");
  TEST_EQUAL(starts, 1, ());
  TEST_EQUAL(m_downloader.GetQueue().Count(), 1, ());
  TEST(m_downloader.GetQueue().Contains("Wonderland_West"), ());
  TEST(Platform::IsFileExistsByFullPath(DownloadingPath("Wonderland_West")), ());
  TEST(!Platform::IsFileExistsByFullPath(DownloadingPath("Wonderland_East")), ());
}

}  // namespace downloader_tests
