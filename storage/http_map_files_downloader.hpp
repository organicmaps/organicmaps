#pragma once

#include "storage/map_files_downloader_with_ping.hpp"
#include "platform/http_request.hpp"
#include "base/thread_checker.hpp"
#include "std/unique_ptr.hpp"

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
  void Download(vector<string> const & urls, string const & path, int64_t size,
                FileDownloadedCallback const & onDownloaded,
                DownloadingProgressCallback const & onProgress) override;

  void OnMapFileDownloaded(FileDownloadedCallback const & onDownloaded,
                           downloader::HttpRequest & request);
  void OnMapFileDownloadingProgress(DownloadingProgressCallback const & onProgress,
                                    downloader::HttpRequest & request);

  unique_ptr<downloader::HttpRequest> m_request;

  DECLARE_THREAD_CHECKER(m_checker);
};
}  // namespace storage
