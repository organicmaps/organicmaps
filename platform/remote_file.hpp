#pragma once

#include "platform/http_client.hpp"

#include <functional>
#include <memory>
#include <string>

namespace platform
{
class RemoteFile
{
public:
  enum class Status
  {
    Ok,
    Forbidden,
    NotFound,
    NetworkError,
    DiskError
  };

  struct Result
  {
    std::string const m_url;
    Status const m_status;
    int const m_httpCode;
    std::string const m_description;

    Result(std::string url, Status status, int httpCode, std::string description)
      : m_url(std::move(url))
      , m_status(status)
      , m_httpCode(httpCode)
      , m_description(std::move(description))
    {}
    Result(std::string url, Status status, std::string description)
      : Result(std::move(url), status, 0, std::move(description))
    {}
  };

  explicit RemoteFile(std::string url, std::string accessToken = {}, HttpClient::Headers const & defaultHeaders = {},
                      bool allowRedirection = true);

  Result Download(std::string const & filePath) const;

  using StartDownloadingHandler = std::function<void(std::string const & filePath)>;
  using ResultHandler = std::function<void(Result &&, std::string const & filePath)>;
  void DownloadAsync(std::string const & filePath, StartDownloadingHandler && startDownloadingHandler,
                     ResultHandler && resultHandler) const;

private:
  std::string const m_url;
  std::string const m_accessToken;
  HttpClient::Headers const m_defaultHeaders;
  bool const m_allowRedirection;
};
}  // namespace platform
