#pragma once

#include "storage/background_downloading/downloader_queue.hpp"
#include "storage/map_files_downloader_with_ping.hpp"

namespace storage
{
class BackgroundDownloaderAdapter : public MapFilesDownloaderWithPing
{
public:
  // MapFilesDownloader overrides:
  void Remove(CountryId const & countryId) override;

  void Clear() override;

  QueueInterface const & GetQueue() const override;

private:
  // MapFilesDownloaderWithServerList overrides:
  void Download(QueuedCountry && queuedCountry) override;

  // Trying to download mwm from different servers recursively.
  void DownloadFromLastUrl(CountryId const & countryId, std::string const & downloadPath,
                           std::vector<std::string> && urls);

  BackgroundDownloaderQueue<uint64_t> m_queue;
};
}  // namespace storage
