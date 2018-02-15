#pragma once

#include "storage/map_files_downloader.hpp"
#include "platform/http_request.hpp"
#include "base/thread_checker.hpp"
#include "std/unique_ptr.hpp"

namespace storage
{
/// This class encapsulates HTTP requests for receiving server lists
/// and file downloading.
//
// *NOTE*, this class is not thread-safe.
class HttpMapFilesDownloader : public MapFilesDownloader
{
public:
  virtual ~HttpMapFilesDownloader();

  // MapFilesDownloader overrides:
  void GetServersList(TServersListCallback const & callback) override;

  void DownloadMapFile(vector<string> const & urls, string const & path, int64_t size,
                       TFileDownloadedCallback const & onDownloaded,
                       TDownloadingProgressCallback const & onProgress) override;
  TProgress GetDownloadingProgress() override;
  bool IsIdle() override;
  void Reset() override;

private:
  void OnServersListDownloaded(TServersListCallback const & callback,
                               downloader::HttpRequest & request);
  void OnMapFileDownloaded(TFileDownloadedCallback const & onDownloaded,
                           downloader::HttpRequest & request);
  void OnMapFileDownloadingProgress(TDownloadingProgressCallback const & onProgress,
                                    downloader::HttpRequest & request);

  unique_ptr<downloader::HttpRequest> m_request;

  DECLARE_THREAD_CHECKER(m_checker);
};
}  // namespace storage
