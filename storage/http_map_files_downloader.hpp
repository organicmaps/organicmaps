#pragma once

#include "storage/downloader_queue_universal.hpp"
#include "storage/map_files_downloader_with_ping.hpp"

#include "platform/http_request.hpp"

#include "base/thread_checker.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace storage
{
/// This class encapsulates HTTP requests for receiving server lists
/// and file downloading.
//
// *NOTE*, this class is not thread-safe.
class HttpMapFilesDownloader : public MapFilesDownloaderWithPing
{
public:
  virtual ~HttpMapFilesDownloader();

  // MapFilesDownloader overrides:
  void Remove(CountryId const & id) override;
  void Clear() override;
  QueueInterface const & GetQueue() const override;

private:
  // MapFilesDownloaderWithServerList overrides:
  void Download(QueuedCountry && queuedCountry) override;

  void Download();

  void OnMapFileDownloaded(QueuedCountry const & queuedCountry, downloader::HttpRequest & request);
  void OnMapFileDownloadingProgress(QueuedCountry const & queuedCountry,
                                    downloader::HttpRequest & request);

  std::unique_ptr<downloader::HttpRequest> m_request;
  Queue m_queue;

  DECLARE_THREAD_CHECKER(m_checker);
};
}  // namespace storage
