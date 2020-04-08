#pragma once

#import "storage/background_downloading/downloader_subscriber_adapter_ios.h"

#include "storage/background_downloading/downloader_queue_ios.hpp"
#include "storage/map_files_downloader_with_ping.hpp"

namespace storage
{
class BackgroundDownloaderAdapter : public storage::MapFilesDownloaderWithPing
{
public:
  BackgroundDownloaderAdapter();

  // MapFilesDownloader overrides:
  void Remove(storage::CountryId const & countryId) override;

  void Clear() override;

  storage::QueueInterface const & GetQueue() const override;

private:
  // MapFilesDownloaderWithServerList overrides:
  void Download(storage::QueuedCountry & queuedCountry) override;

  // Trying to download mwm from different servers recursively.
  void DownloadFromAnyUrl(CountryId const & countryId, std::string const & downloadPath,
                          std::vector<std::string> const & urls);

  storage::BackgroundDownloaderQueue m_queue;
  SubscriberAdapter * m_subscriberAdapter;
};
}  // namespace storage
