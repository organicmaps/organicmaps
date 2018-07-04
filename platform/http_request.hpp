#pragma once

#include "std/cstdint.hpp"
#include "std/function.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"
#include "std/utility.hpp"

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
  enum class Status
  {
    InProgress,
    Completed,
    Failed,
    FileNotFound
  };

  /// <current, total>, total can be -1 if size is unknown
  using Progress = pair<int64_t, int64_t>;
  using Callback = function<void(HttpRequest & request)>;

protected:
  Status m_status;
  Progress m_progress;
  Callback m_onFinish;
  Callback m_onProgress;

  HttpRequest(Callback const & onFinish, Callback const & onProgress);

public:
  virtual ~HttpRequest() = 0;

  Status GetStatus() const { return m_status; }
  Progress const & GetProgress() const { return m_progress; }
  /// Either file path (for chunks) or downloaded data
  virtual string const & GetData() const = 0;

  /// Response saved to memory buffer and retrieved with Data()
  static HttpRequest * Get(string const & url,
                           Callback const & onFinish,
                           Callback const & onProgress = Callback());

  /// Content-type for request is always "application/json"
  static HttpRequest * PostJson(string const & url, string const & postData,
                                Callback const & onFinish,
                                Callback const & onProgress = Callback());

  /// Download file to filePath.
  /// @param[in]  fileSize  Correct file size (needed for resuming and reserving).
  static HttpRequest * GetFile(vector<string> const & urls,
                               string const & filePath, int64_t fileSize,
                               Callback const & onFinish,
                               Callback const & onProgress = Callback(),
                               int64_t chunkSize = 512 * 1024,
                               bool doCleanOnCancel = true);
};

string DebugPrint(HttpRequest::Status status);
} // namespace downloader
