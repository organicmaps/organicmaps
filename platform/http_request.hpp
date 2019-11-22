#pragma once

#include "platform/downloader_defines.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace downloader
{
namespace non_http_error_code
{
auto constexpr kIOException = -1;
auto constexpr kWriteException = -2;
auto constexpr kInconsistentFileSize = -3;
auto constexpr kNonHttpResponse = -4;
auto constexpr kInvalidURL = -5;
auto constexpr kCancelled = -6;
}  // namespace non_http_error_code

/// Request in progress will be canceled on delete
class HttpRequest
{
public:
  using Callback = std::function<void(HttpRequest & request)>;

protected:
  DownloadStatus m_status;
  Progress m_progress;
  Callback m_onFinish;
  Callback m_onProgress;

  HttpRequest(Callback const & onFinish, Callback const & onProgress);

public:
  virtual ~HttpRequest() = 0;

  DownloadStatus GetStatus() const { return m_status; }
  Progress const & GetProgress() const { return m_progress; }
  /// Either file path (for chunks) or downloaded data
  virtual std::string const & GetData() const = 0;

  /// Response saved to memory buffer and retrieved with Data()
  static HttpRequest * Get(std::string const & url,
                           Callback const & onFinish,
                           Callback const & onProgress = Callback());

  /// Content-type for request is always "application/json"
  static HttpRequest * PostJson(std::string const & url, std::string const & postData,
                                Callback const & onFinish,
                                Callback const & onProgress = Callback());

  /// Download file to filePath.
  /// @param[in]  fileSize  Correct file size (needed for resuming and reserving).
  static HttpRequest * GetFile(std::vector<std::string> const & urls,
                               std::string const & filePath, int64_t fileSize,
                               Callback const & onFinish,
                               Callback const & onProgress = Callback(),
                               int64_t chunkSize = 512 * 1024,
                               bool doCleanOnCancel = true);
};
} // namespace downloader
