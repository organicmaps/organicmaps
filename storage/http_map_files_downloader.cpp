#include "storage/http_map_files_downloader.hpp"

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
  explicit ErrorHttpRequest(std::string const & filePath)
  : HttpRequest(Callback(), Callback()), m_filePath(filePath)
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

void HttpMapFilesDownloader::Download(QueuedCountry & queuedCountry)
{
  CHECK_THREAD_CHECKER(m_checker, ());

  m_queue.emplace_back(std::move(queuedCountry));

  for (auto const subscriber : m_subscribers)
    subscriber->OnCountryInQueue(queuedCountry.GetCountryId());

  if (m_queue.size() != 1)
    return;

  for (auto const subscriber : m_subscribers)
    subscriber->OnStartDownloading();

  Download();
}

void HttpMapFilesDownloader::Download()
{
  CHECK_THREAD_CHECKER(m_checker, ());

  auto & queuedCountry = m_queue.front();

  queuedCountry.ClarifyDownloadingType();

  auto const urls = MakeUrlList(queuedCountry.GetRelativeUrl());
  auto const path = queuedCountry.GetFileDownloadPath();
  auto const size = queuedCountry.GetDownloadSize();

  m_request.reset();

  if (IsDownloadingAllowed())
  {
    for (auto const subscriber : m_subscribers)
      subscriber->OnStartDownloadingCountry(queuedCountry.GetCountryId());

    m_request.reset(downloader::HttpRequest::GetFile(
        urls, path, size,
        std::bind(&HttpMapFilesDownloader::OnMapFileDownloaded, this, queuedCountry, _1),
        std::bind(&HttpMapFilesDownloader::OnMapFileDownloadingProgress, this, queuedCountry, _1)));
  }
  else
  {
    ErrorHttpRequest error(path);
    auto const copy = queuedCountry;
    OnMapFileDownloaded(copy, error);
  }
}

downloader::Progress HttpMapFilesDownloader::GetDownloadingProgress()
{
  CHECK_THREAD_CHECKER(m_checker, ());
  ASSERT(nullptr != m_request, ());
  return m_request->GetProgress();
}

bool HttpMapFilesDownloader::IsIdle()
{
  CHECK_THREAD_CHECKER(m_checker, ());
  return m_request.get() == nullptr;
}

void HttpMapFilesDownloader::Pause()
{
  CHECK_THREAD_CHECKER(m_checker, ());
  m_request.reset();
}

void HttpMapFilesDownloader::Resume()
{
  CHECK_THREAD_CHECKER(m_checker, ());

  if (!m_request && !m_queue.empty())
    Download();
}

void HttpMapFilesDownloader::Remove(CountryId const & id)
{
  CHECK_THREAD_CHECKER(m_checker, ());

  MapFilesDownloader::Remove(id);

  if (m_queue.empty())
    return;

  if (m_request && m_queue.front() == id)
    m_request.reset();

  auto it = std::find(m_queue.begin(), m_queue.end(), id);
  if (it != m_queue.end())
    m_queue.erase(it);

  if (m_queue.empty())
  {
    for (auto const subscriber : m_subscribers)
      subscriber->OnFinishDownloading();
  }
}

void HttpMapFilesDownloader::Clear()
{
  CHECK_THREAD_CHECKER(m_checker, ());

  MapFilesDownloader::Clear();

  auto needNotify = m_request != nullptr;

  m_request.reset();
  m_queue.clear();

  if (needNotify)
  {
    for (auto const subscriber : m_subscribers)
      subscriber->OnFinishDownloading();
  }
}

Queue const & HttpMapFilesDownloader::GetQueue() const
{
  CHECK_THREAD_CHECKER(m_checker, ());

  if (m_queue.empty())
    return MapFilesDownloader::GetQueue();

  return m_queue;
}

void HttpMapFilesDownloader::OnMapFileDownloaded(QueuedCountry const & queuedCountry,
                                                 downloader::HttpRequest & request)
{
  CHECK_THREAD_CHECKER(m_checker, ());
  // Because this method is called deferred on original thread,
  // it is possible the country is already removed from queue.
  if (m_queue.empty() || m_queue.front().GetCountryId() != queuedCountry.GetCountryId())
    return;

  m_queue.pop_front();

  queuedCountry.OnDownloadFinished(request.GetStatus());

  m_request.reset();

  if (!m_queue.empty())
  {
    Download();
  }
  else
  {
    for (auto const subscriber : m_subscribers)
      subscriber->OnFinishDownloading();
  }
}

void HttpMapFilesDownloader::OnMapFileDownloadingProgress(QueuedCountry const & queuedCountry,
                                                          downloader::HttpRequest & request)
{
  CHECK_THREAD_CHECKER(m_checker, ());
  // Because of this method calls deferred on original thread,
  // it is possible the country is already removed from queue.
  if (m_queue.empty() || m_queue.front().GetCountryId() != queuedCountry.GetCountryId())
    return;

  queuedCountry.OnDownloadProgress(request.GetProgress());
}
}  // namespace storage
