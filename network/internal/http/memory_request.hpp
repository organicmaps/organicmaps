#pragma once

#include "coding/writer.hpp"

#include "network/download_status.hpp"
#include "network/http/request.hpp"
#include "network/http/thread.hpp"
#include "network/non_http_error_code.hpp"

namespace om::network::http::internal
{
/// Stores server response into the memory
class MemoryRequest : public Request, public IThreadCallback
{
public:
  MemoryRequest(std::string const & url, Callback && onFinish, Callback && onProgress)
    : Request(std::move(onFinish), std::move(onProgress)), m_requestUrl(url), m_writer(m_downloadedData)
  {
    m_thread = thread::CreateThread(url, *this);
    ASSERT(m_thread, ());
  }

  MemoryRequest(std::string const & url, std::string const & postData, Callback && onFinish, Callback && onProgress)
    : Request(std::move(onFinish), std::move(onProgress)), m_writer(m_downloadedData)
  {
    m_thread = thread::CreateThread(url, *this, 0, -1, -1, postData);
    ASSERT(m_thread, ());
  }

  ~MemoryRequest() override { thread::DeleteThread(m_thread); }

  std::string const & GetData() const override { return m_downloadedData; }

private:
  bool OnWrite(int64_t, void const * buffer, size_t size) override
  {
    m_writer.Write(buffer, size);
    m_progress.m_bytesDownloaded += size;
    if (m_onProgress)
      m_onProgress(*this);
    return true;
  }

  void OnFinish(long httpOrErrorCode, int64_t, int64_t) override
  {
    if (httpOrErrorCode == 200)
    {
      m_status = DownloadStatus::Completed;
    }
    else
    {
      auto const message = non_http_error_code::DebugPrint(httpOrErrorCode);
      LOG(LWARNING, ("HttpRequest error:", message));
      if (httpOrErrorCode == 404)
        m_status = DownloadStatus::FileNotFound;
      else
        m_status = DownloadStatus::Failed;
    }

    m_onFinish(*this);
  }

  Thread * m_thread;

  std::string m_requestUrl;
  std::string m_downloadedData;
  MemWriter<std::string> m_writer;
};
}  // namespace om::network::http::internal
