#include "storage/http_map_files_downloader.hpp"

#include "storage/downloader.hpp"

#include "platform/downloader_defines.hpp"
#include "platform/servers_list.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <functional>

using namespace std::placeholders;

namespace
{
class ErrorHttpRequest : public downloader::HttpRequest
{
public:
  explicit ErrorHttpRequest(std::string const & filePath) : HttpRequest(Callback(), Callback()), m_filePath(filePath)
  {
    m_status = downloader::DownloadStatus::Failed;
  }

  virtual std::string const & GetData() const { return m_filePath; }

private:
  std::string m_filePath;
};
}  // anonymous namespace

namespace storage
{
HttpMapFilesDownloader::~HttpMapFilesDownloader()
{
  CHECK_THREAD_CHECKER(m_checker, ());
}

void HttpMapFilesDownloader::Download(QueuedCountry && queuedCountry)
{
  CHECK_THREAD_CHECKER(m_checker, ());

  m_queue.Append(std::move(queuedCountry));

  if (m_queue.Count() == 1)
    Download();
}

void HttpMapFilesDownloader::Download()
{
  CHECK_THREAD_CHECKER(m_checker, ());

  auto const & queuedCountry = m_queue.GetFirstCountry();

  auto const urls = MakeUrlList(queuedCountry.GetRelativeUrl());
  auto const path = queuedCountry.GetFileDownloadPath();
  auto const size = queuedCountry.GetDownloadSize();

  m_request.reset();

  if (IsDownloadingAllowed())
  {
    queuedCountry.OnStartDownloading();

    m_request.reset(downloader::HttpRequest::GetFile(
        urls, path, size, std::bind(&HttpMapFilesDownloader::OnMapFileDownloaded, this, queuedCountry, _1),
        std::bind(&HttpMapFilesDownloader::OnMapFileDownloadingProgress, this, queuedCountry, _1)));
  }
  else
  {
    ErrorHttpRequest error(path);
    auto const copy = queuedCountry;
    OnMapFileDownloaded(copy, error);
  }
}

void HttpMapFilesDownloader::Remove(CountryId const & id)
{
  CHECK_THREAD_CHECKER(m_checker, ());

  MapFilesDownloader::Remove(id);

  if (!m_queue.Contains(id))
    return;

  if (m_request && m_queue.GetFirstId() == id)
    m_request.reset();

  m_queue.Remove(id);

  if (!m_queue.IsEmpty() && !m_request)
    Download();
}

void HttpMapFilesDownloader::Clear()
{
  CHECK_THREAD_CHECKER(m_checker, ());

  MapFilesDownloader::Clear();

  m_request.reset();
  m_queue.Clear();
}

QueueInterface const & HttpMapFilesDownloader::GetQueue() const
{
  CHECK_THREAD_CHECKER(m_checker, ());

  if (m_queue.IsEmpty())
    return MapFilesDownloader::GetQueue();

  return m_queue;
}

void HttpMapFilesDownloader::OnMapFileDownloaded(QueuedCountry const & queuedCountry, downloader::HttpRequest & request)
{
  CHECK_THREAD_CHECKER(m_checker, ());
  // Because this method is called deferred on original thread,
  // it is possible the country is already removed from queue.
  if (m_queue.IsEmpty() || m_queue.GetFirstId() != queuedCountry.GetCountryId())
    return;

  m_queue.PopFront();

  queuedCountry.OnDownloadFinished(request.GetStatus());

  m_request.reset();

  if (!m_queue.IsEmpty())
    Download();
}

void HttpMapFilesDownloader::OnMapFileDownloadingProgress(QueuedCountry const & queuedCountry,
                                                          downloader::HttpRequest & request)
{
  CHECK_THREAD_CHECKER(m_checker, ());
  // Because of this method calls deferred on original thread,
  // it is possible the country is already removed from queue.
  if (m_queue.IsEmpty() || m_queue.GetFirstId() != queuedCountry.GetCountryId())
    return;

  queuedCountry.OnDownloadProgress(request.GetProgress());
}

std::unique_ptr<MapFilesDownloader> GetDownloader()
{
  return std::make_unique<HttpMapFilesDownloader>();
}
}  // namespace storage
