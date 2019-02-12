#pragma once

#include "platform/http_request.hpp"

#include "std/function.hpp"
#include "std/string.hpp"

#include "std/utility.hpp"
#include "std/vector.hpp"

namespace storage
{
// This interface encapsulates HTTP routines for receiving servers
// URLs and downloading a single map file.
class MapFilesDownloader
{
public:
  // Denotes bytes downloaded and total number of bytes.
  using Progress = pair<int64_t, int64_t>;

  using FileDownloadedCallback =
      function<void(downloader::HttpRequest::Status status, Progress const & progress)>;
  using DownloadingProgressCallback = function<void(Progress const & progress)>;
  using ServersListCallback = function<void(vector<string> & urls)>;

  virtual ~MapFilesDownloader() = default;

  /// Asynchronously receives a list of all servers that can be asked
  /// for a map file and invokes callback on the original thread.
  virtual void GetServersList(ServersListCallback const & callback) = 0;

  /// Asynchronously downloads a map file, periodically invokes
  /// onProgress callback and finally invokes onDownloaded
  /// callback. Both callbacks will be invoked on the original thread.
  virtual void DownloadMapFile(vector<string> const & urls, string const & path, int64_t size,
                               FileDownloadedCallback const & onDownloaded,
                               DownloadingProgressCallback const & onProgress) = 0;

  /// Returns current downloading progress.
  virtual Progress GetDownloadingProgress() = 0;

  /// Returns true when downloader does not perform any job.
  virtual bool IsIdle() = 0;

  /// Resets downloader to the idle state.
  virtual void Reset() = 0;
};
}  // namespace storage
