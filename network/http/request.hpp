#pragma once

#include "network/download_status.hpp"
#include "network/progress.hpp"

namespace om::network::http
{
class Request
{
public:
  using Callback = std::function<void(Request & request)>;

public:
  virtual ~Request() = default;

  DownloadStatus GetStatus() const { return m_status; }
  Progress const & GetProgress() const { return m_progress; }
  /// Either file path (for chunks) or downloaded data
  virtual std::string const & GetData() const = 0;

  /// Response saved to memory buffer and retrieved with Data()
  static Request * Get(std::string const & url, Callback && onFinish, Callback && onProgress = Callback());

  /// Content-type for request is always "application/json"
  static Request * PostJson(std::string const & url, std::string const & postData, Callback && onFinish,
                            Callback && onProgress = Callback());

  /// Download file to filePath.
  /// @param[in]  fileSize  Correct file size (needed for resuming and reserving).
  static Request * GetFile(std::vector<std::string> const & urls, std::string const & filePath, int64_t fileSize,
                           Callback && onFinish, Callback && onProgress = Callback(), int64_t chunkSize = 512 * 1024,
                           bool doCleanOnCancel = true);

protected:
  DownloadStatus m_status;
  Progress m_progress;
  Callback m_onFinish;
  Callback m_onProgress;

  Request(Callback && onFinish, Callback && onProgress);
};
}  // namespace om::network::http
