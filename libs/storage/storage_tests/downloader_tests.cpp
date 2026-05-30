#include "testing/testing.hpp"

#include "storage/map_files_downloader_with_ping.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"

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

}  // namespace downloader_tests
