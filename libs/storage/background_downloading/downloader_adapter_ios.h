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
  // MapFilesDownloader overrides:
  void Download(QueuedCountry && queuedCountry) override;

  // Retries mirrors only while this queue entry is still active.
  void DownloadFromLastUrl(CountryId const & countryId, std::string const & downloadPath,
                           std::vector<std::string> && urls, std::weak_ptr<int> lifetime);

  BackgroundDownloaderQueue<uint64_t> m_queue;
};
}  // namespace storage
