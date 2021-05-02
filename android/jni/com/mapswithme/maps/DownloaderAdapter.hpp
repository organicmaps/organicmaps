#pragma once

#include "storage/background_downloading/downloader_queue.hpp"
#include "storage/storage_defines.hpp"
#include "storage/map_files_downloader_with_ping.hpp"

#include "base/thread_checker.hpp"

#include <jni.h>

#include <cstdint>
#include <memory>
#include <optional>

namespace storage
{
class BackgroundDownloaderAdapter : public MapFilesDownloaderWithPing
{
public:
  BackgroundDownloaderAdapter();
  ~BackgroundDownloaderAdapter();

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

  void RemoveByRequestId(int64_t id);

  BackgroundDownloaderQueue<int64_t> m_queue;

  jmethodID m_downloadManagerRemove = nullptr;
  jmethodID m_downloadManagerEnqueue = nullptr;
  std::shared_ptr<jobject> m_downloadManager;

  DECLARE_THREAD_CHECKER(m_threadChecker);
};
}  // namespace storage
