#include "platform/remote_file.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"

#include "coding/file_writer.hpp"

#include "base/logging.hpp"

namespace platform
{
namespace
{
double constexpr kRequestTimeoutInSec = 5.0;
}  // namespace

RemoteFile::RemoteFile(std::string url, std::string accessToken /* = {} */,
                       HttpClient::Headers const & defaultHeaders /* = {} */, bool allowRedirection /* = true */)
  : m_url(std::move(url))
  , m_accessToken(std::move(accessToken))
  , m_defaultHeaders(defaultHeaders)
  , m_allowRedirection(allowRedirection)
{}

RemoteFile::Result RemoteFile::Download(std::string const & filePath) const
{
  if (m_url.empty())
    return {m_url, Status::NetworkError, "Empty URL"};

  platform::HttpClient request(m_url);
  request.SetRawHeaders(m_defaultHeaders);
  request.SetTimeout(kRequestTimeoutInSec);
  request.SetFollowRedirects(m_allowRedirection);
  if (!m_accessToken.empty())
    request.SetRawHeader("Authorization", "Bearer " + m_accessToken);
  if (request.RunHttpRequest())
  {
    if (!m_allowRedirection && request.WasRedirected())
      return {m_url, Status::NetworkError, "Unexpected redirection"};

    auto const & response = request.ServerResponse();
    int const resultCode = request.ErrorCode();
    if (resultCode == 403)
    {
      LOG(LWARNING, ("Access denied for", m_url, "response:", response));
      return {m_url, Status::Forbidden, resultCode, response};
    }
    else if (resultCode == 404)
    {
      LOG(LWARNING, ("File not found at", m_url, "response:", response));
      return {m_url, Status::NotFound, resultCode, response};
    }

    if (resultCode >= 200 && resultCode < 300)
    {
      try
      {
        FileWriter w(filePath);
        w.Write(response.data(), response.length());
      }
      catch (FileWriter::Exception const & exception)
      {
        LOG(LWARNING, ("Exception while writing file:", filePath, "reason:", exception.what()));
        return {m_url, Status::DiskError, resultCode, exception.what()};
      }
      return {m_url, Status::Ok, resultCode, ""};
    }
    return {m_url, Status::NetworkError, resultCode, response};
  }
  return {m_url, Status::NetworkError, "Unspecified network error"};
}

void RemoteFile::DownloadAsync(std::string const & filePath, StartDownloadingHandler && startDownloadingHandler,
                               ResultHandler && resultHandler) const
{
  RemoteFile remoteFile = *this;
  GetPlatform().RunTask(Platform::Thread::Network, [filePath, remoteFile = std::move(remoteFile),
                                                    startDownloadingHandler = std::move(startDownloadingHandler),
                                                    resultHandler = std::move(resultHandler)]
  {
    if (startDownloadingHandler)
      startDownloadingHandler(filePath);
    auto result = remoteFile.Download(filePath);
    if (resultHandler)
      resultHandler(std::move(result), filePath);
  });
}
}  // namespace platform
