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

RemoteFile::RemoteFile(std::string url, bool allowRedirection)
  : m_url(std::move(url))
  , m_allowRedirection(allowRedirection)
{}

RemoteFile::Result RemoteFile::Download(std::string const & filePath) const
{
  platform::HttpClient request(m_url);
  request.SetTimeout(kRequestTimeoutInSec);
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

void RemoteFile::DownloadAsync(std::string const & filePath, ResultHandler && handler) const
{
  RemoteFile remoteFile = *this;
  GetPlatform().RunTask(Platform::Thread::Network,
                        [filePath, remoteFile = std::move(remoteFile), handler = std::move(handler)]()
  {
    auto result = remoteFile.Download(filePath);
    if (handler)
      handler(std::move(result), filePath);
  });
}
}  // namespace platform
