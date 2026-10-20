#include "testing/testing.hpp"

#include "storage/map_files_downloader_with_ping.hpp"

#include "platform/platform.hpp"
#include "platform/platform_tests_support/writable_dir_changer.hpp"

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

class ConfiguredDownloaderStub : public DownloaderStub
{
public:
  downloader::MetaConfig GetMetaConfig() override { return m_config; }

  void LoadConfig()
  {
    EnsureMetaConfigReady([] { testing::StopEventLoop(); });
    testing::RunEventLoop();
  }

  downloader::MetaConfig m_config;
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
  urls = downloader.MakeUrlList("terrain/2/N40E040.twm");
  TEST_EQUAL(urls, (storage::MapFilesDownloader::ServersList{"https://example.com/base/terrain/2/N40E040.twm"}), ());

  downloader.ResetServersList();
  TEST(downloader.MakeUrlList("maps/1/Germany.mwm").empty(), ());
  TEST(downloader.MakeUrlList("terrain/2/N40E040.twm").empty(), ());
}

UNIT_TEST(MapFilesDownloader_TerrainServers)
{
  WritableDirChanger const dir("terrain-servers", WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  Platform::ThreadRunner runner;
  ConfiguredDownloaderStub downloader;
  // A grid release can reference blocks from an older terrain version.
  downloader.SetTerrainDataVersion(3);
  downloader.m_config.servers = {"https://maps.example/"};
  downloader.m_config.terrainServers = {"https://terrain1.example/", "https://terrain2.example/"};
  downloader.LoadConfig();

  TEST_EQUAL(downloader.MakeUrlList("maps/1/Germany.mwm"),
             (storage::MapFilesDownloader::ServersList{"https://maps.example/maps/1/Germany.mwm"}), ());
  TEST_EQUAL(downloader.MakeUrlList("diffs/1/0/Germany.mwmdiff"),
             (storage::MapFilesDownloader::ServersList{"https://maps.example/diffs/1/0/Germany.mwmdiff"}), ());
  TEST_EQUAL(downloader.MakeUrlList("terrain/2/N40E040.twm"),
             (storage::MapFilesDownloader::ServersList{"https://terrain1.example/terrain/2/N40E040.twm",
                                                       "https://terrain2.example/terrain/2/N40E040.twm"}),
             ());

  downloader.SetServersList({"https://debug.example/"});
  TEST_EQUAL(downloader.MakeUrlList("terrain/2/N40E040.twm"),
             (storage::MapFilesDownloader::ServersList{"https://debug.example/terrain/2/N40E040.twm"}), ());
  downloader.ResetServersList();
  downloader.LoadConfig();
  TEST_EQUAL(downloader.MakeUrlList("terrain/2/N40E040.twm").size(), 2, ());
  TEST_EQUAL(downloader.MakeUrlList("maps/1/Germany.mwm"),
             (storage::MapFilesDownloader::ServersList{"https://maps.example/maps/1/Germany.mwm"}), ());
}

UNIT_TEST(MapFilesDownloader_LegacyTerrainServers)
{
  WritableDirChanger const dir("legacy-terrain-servers", WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  Platform::ThreadRunner runner;
  ConfiguredDownloaderStub downloader;
  downloader.m_config.servers = {"https://maps.example/"};
  downloader.LoadConfig();
  TEST_EQUAL(downloader.MakeUrlList("terrain/2/N40E040.twm"),
             (storage::MapFilesDownloader::ServersList{"https://cdn.organicmaps.app/terrain/2/N40E040.twm"}), ());
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
