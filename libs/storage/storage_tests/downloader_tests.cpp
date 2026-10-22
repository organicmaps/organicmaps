#include "testing/testing.hpp"

#include "storage/map_files_downloader_with_ping.hpp"
#include "storage/storage_tests/test_map_files_downloader.hpp"

#include "platform/local_country_file_utils.hpp"
#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/writable_dir_changer.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <utility>
#include <vector>

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

class CallbackDownloader : public storage::TestMapFilesDownloader
{
  downloader::MetaConfig GetMetaConfig() override { return {}; }
};

class HttpDownloaderTest : public storage::QueuedCountry::Subscriber
{
public:
  ~HttpDownloaderTest() override { m_downloader.Clear(); }

  void OnCountryInQueue(storage::QueuedCountry const & country) override
  {
    if (m_onQueue)
      m_onQueue(country);
  }
  void OnStartDownloading(storage::QueuedCountry const & country) override
  {
    if (m_onStart)
      m_onStart(country);
  }
  void OnDownloadProgress(storage::QueuedCountry const &, downloader::Progress const &) override {}
  void OnDownloadFinished(storage::QueuedCountry const & country, downloader::DownloadStatus status) override
  {
    if (m_onFinish)
    {
      m_onFinish(country, status);
      return;
    }
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

  // StringStorage caches its path, so settings must stay in CTest's stable directory.
  WritableDirChanger const m_dir{"http-downloader-tests"};
  Platform::ThreadRunner m_threads;
  CallbackDownloader m_downloader;
  std::function<void(storage::QueuedCountry const &)> m_onStart;
  std::function<void(storage::QueuedCountry const &)> m_onQueue;
  std::function<void(storage::QueuedCountry const &, downloader::DownloadStatus)> m_onFinish;
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

UNIT_CLASS_TEST(HttpDownloaderTest, ReplaceFromQueueCallback)
{
  for (bool const clear : {false, true})
  {
    size_t starts = 0;
    bool replaced = false;
    auto const dataVersion = version::FOR_TESTING_MWM1 + (clear ? 1 : 0);
    m_onStart = [&](storage::QueuedCountry const &) { ++starts; };
    m_onQueue = [&](storage::QueuedCountry const & country)
    {
      if (replaced)
        return;
      replaced = true;
      if (clear)
        m_downloader.Clear();
      else
        m_downloader.Remove(country.GetCountryId());
      Enqueue(country.GetCountryId(), dataVersion);
    };
    Enqueue("Wonderland_West", dataVersion);
    TEST(replaced, (clear));
    TEST_EQUAL(starts, 1, (clear));
    TEST_EQUAL(m_downloader.GetQueue().Count(), 1, (clear));
    TEST(m_downloader.GetQueue().Contains("Wonderland_West"), (clear));
    m_downloader.Clear();
  }
}

UNIT_CLASS_TEST(HttpDownloaderTest, ClearWhileReceivingServerList)
{
  m_downloader.ResetServersList();
  Enqueue("Wonderland_West");
  Enqueue("Wonderland_East");
  size_t notifications = 0;
  m_onQueue = [&](storage::QueuedCountry const & country)
  {
    ++notifications;
    m_downloader.Clear();
    TEST_EQUAL(country.GetCountryId(), "Wonderland_West", ());
  };
  m_downloader.SetServersList({"http://localhost:24568/unit_tests/"});
  std::promise<void> networkIdle;
  TEST(GetPlatform().RunTask(Platform::Thread::Network, [&]() { networkIdle.set_value(); }).m_isSuccess, ());
  networkIdle.get_future().wait();
  TEST_EQUAL(notifications, 1, ());
  TEST(m_downloader.GetQueue().IsEmpty(), ());
}

UNIT_CLASS_TEST(HttpDownloaderTest, AdvanceAfterRequestCreationFailure)
{
  std::vector<storage::CountryId> started;
  std::vector<storage::CountryId> finished;
  m_onStart = [&](storage::QueuedCountry const & country)
  {
    started.push_back(country.GetCountryId());
    if (country.GetCountryId() != "Wonderland_West")
      return;
    Enqueue("Wonderland_East", version::FOR_TESTING_MWM1 + 1);
    TEST_EQUAL(Platform::RmDir(base::GetDirectory(country.GetFileDownloadPath())), Platform::ERR_OK, ());
  };
  m_onFinish = [&](storage::QueuedCountry const & country, downloader::DownloadStatus status)
  {
    TEST_EQUAL(status, downloader::DownloadStatus::Failed, ());
    finished.push_back(country.GetCountryId());
  };
  Enqueue("Wonderland_West");
  TEST_EQUAL(started, (std::vector<storage::CountryId>{"Wonderland_West", "Wonderland_East"}), ());
  TEST_EQUAL(finished, (std::vector<storage::CountryId>{"Wonderland_West"}), ());
  TEST_EQUAL(m_downloader.GetQueue().Count(), 1, ());
  TEST(m_downloader.GetQueue().Contains("Wonderland_East"), ());
}

UNIT_CLASS_TEST(HttpDownloaderTest, EnqueueFromFinishCallback)
{
  size_t starts = 0;
  m_onStart = [&](storage::QueuedCountry const & country)
  {
    if (country.GetCountryId() == "Wonderland_East")
      ++starts;
  };
  m_onFinish = [&](storage::QueuedCountry const & country, downloader::DownloadStatus status)
  {
    if (country.GetCountryId() == "Wonderland_East")
      return;
    TEST_EQUAL(country.GetCountryId(), "Uruguay", ());
    TEST_EQUAL(status, downloader::DownloadStatus::FileNotFound, ());
    Enqueue("Wonderland_East");
    TEST_EQUAL(country.GetCountryId(), "Uruguay", ());
    testing::StopEventLoop();
  };
  Enqueue("Uruguay", 1234);
  testing::RunEventLoop();
  TEST_EQUAL(starts, 1, ());
}
}  // namespace downloader_tests
