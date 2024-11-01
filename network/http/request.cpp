#include "request.hpp"

#include "base/logging.hpp"

#include "network/internal/http/file_request.hpp"
#include "network/internal/http/memory_request.hpp"

namespace om::network::http
{
Request::Request(Callback && onFinish, Callback && onProgress)
  : m_status(DownloadStatus::InProgress)
  , m_progress(Progress::Unknown())
  , m_onFinish(std::move(onFinish))
  , m_onProgress(std::move(onProgress))
{
}

Request * Request::Get(std::string const & url, Callback && onFinish, Callback && onProgress)
{
  return new internal::MemoryRequest(url, std::move(onFinish), std::move(onProgress));
}

Request * Request::PostJson(std::string const & url, std::string const & postData, Callback && onFinish,
                            Callback && onProgress)
{
  return new internal::MemoryRequest(url, postData, std::move(onFinish), std::move(onProgress));
}

Request * Request::GetFile(std::vector<std::string> const & urls, std::string const & filePath, int64_t fileSize,
                           Callback && onFinish, Callback && onProgress, int64_t chunkSize, bool doCleanOnCancel)
{
  try
  {
    return new internal::FileRequest(urls, filePath, fileSize, std::move(onFinish), std::move(onProgress), chunkSize,
                                     doCleanOnCancel);
  }
  catch (FileWriter::Exception const & e)
  {
    // Can't create or open file for writing.
    LOG(LWARNING, ("Can't create file", filePath, "with size", fileSize, e.Msg()));
  }
  return nullptr;
}
}  // namespace om::network::http
