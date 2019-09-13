#pragma once

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
  Progress GetDownloadingProgress() override;
  bool IsIdle() override;
  void Reset() override;

private:
  // MapFilesDownloaderWithServerList overrides:
  void Download(std::vector<std::string> const & urls, std::string const & path, int64_t size,
                FileDownloadedCallback const & onDownloaded,
                DownloadingProgressCallback const & onProgress) override;

  void OnMapFileDownloaded(FileDownloadedCallback const & onDownloaded,
                           downloader::HttpRequest & request);
  void OnMapFileDownloadingProgress(DownloadingProgressCallback const & onProgress,
                                    downloader::HttpRequest & request);

  std::unique_ptr<downloader::HttpRequest> m_request;

  DECLARE_THREAD_CHECKER(m_checker);
};
}  // namespace storage
