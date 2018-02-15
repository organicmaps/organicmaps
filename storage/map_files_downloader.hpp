#pragma once

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
  using TProgress = pair<int64_t, int64_t>;

  using TFileDownloadedCallback = function<void(bool success, TProgress const & progress)>;
  using TDownloadingProgressCallback = function<void(TProgress const & progress)>;
  using TServersListCallback = function<void(vector<string> & urls)>;

  virtual ~MapFilesDownloader() = default;

  /// Asynchronously receives a list of all servers that can be asked
  /// for a map file and invokes callback on the original thread.
  virtual void GetServersList(TServersListCallback const & callback) = 0;

  /// Asynchronously downloads a map file, periodically invokes
  /// onProgress callback and finally invokes onDownloaded
  /// callback. Both callbacks will be invoked on the original thread.
  virtual void DownloadMapFile(vector<string> const & urls, string const & path, int64_t size,
                               TFileDownloadedCallback const & onDownloaded,
                               TDownloadingProgressCallback const & onProgress) = 0;

  /// Returns current downloading progress.
  virtual TProgress GetDownloadingProgress() = 0;

  /// Returns true when downloader does not perform any job.
  virtual bool IsIdle() = 0;

  /// Resets downloader to the idle state.
  virtual void Reset() = 0;
};
}  // namespace storage
