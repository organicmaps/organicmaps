#import "storage/background_downloading/downloader_adapter_ios.h"

#import "platform/background_downloader_ios.h"

#include "storage/downloader.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"

#include <memory>
#include <utility>

namespace storage
{
BackgroundDownloaderAdapter::BackgroundDownloaderAdapter()
{
  m_subscriberAdapter = [[SubscriberAdapter alloc] initWithSubscribers:m_subscribers];
  BackgroundDownloader * downloader = [BackgroundDownloader sharedBackgroundMapDownloader];
  [downloader subscribe:m_subscriberAdapter];
}

void BackgroundDownloaderAdapter::Remove(storage::CountryId const & countryId)
{
  MapFilesDownloader::Remove(countryId);
  
  if (!m_queue.Contains(countryId))
    return;

  BackgroundDownloader * downloader = [BackgroundDownloader sharedBackgroundMapDownloader];
  auto const taskIdentifier = m_queue.GetTaskIdByCountryId(countryId);
  if (taskIdentifier)
    [downloader cancelTaskWithIdentifier:*taskIdentifier];
  m_queue.Remove(countryId);
}

void BackgroundDownloaderAdapter::Clear()
{
  MapFilesDownloader::Clear();
  
  BackgroundDownloader * downloader = [BackgroundDownloader sharedBackgroundMapDownloader];
  [downloader clear];
  m_queue.Clear();
};

storage::QueueInterface const & BackgroundDownloaderAdapter::GetQueue() const
{
  if (m_queue.IsEmpty())
    return MapFilesDownloader::GetQueue();
    
  return m_queue;
}

void BackgroundDownloaderAdapter::Download(storage::QueuedCountry & queuedCountry)
{
  if (!IsDownloadingAllowed())
  {
    queuedCountry.OnDownloadFinished(downloader::DownloadStatus::Failed);
    if (m_queue.IsEmpty())
    {
      for (auto const & subscriber : m_subscribers)
        subscriber->OnFinishDownloading();
    }
    return;
  }

  auto const countryId = queuedCountry.GetCountryId();
  auto const urls = MakeUrlList(queuedCountry.GetRelativeUrl());
  
  m_queue.Append(std::move(queuedCountry));

  DownloadFromAnyUrl(countryId, queuedCountry.GetFileDownloadPath(), urls);
}

void BackgroundDownloaderAdapter::DownloadFromAnyUrl(CountryId const & countryId,
                                                     std::string const & downloadPath,
                                                     std::vector<std::string> const & urls)
{
  if (urls.empty())
    return;
  
  auto onFinish = [this, countryId, downloadPath, urls = urls](NSURL *location, NSError *error) mutable {
    if ((!location && !error) || (error && error.code != NSURLErrorCancelled))
      return;
     
    downloader::DownloadStatus status = downloader::DownloadStatus::Completed;
    if (error)
    {
     status = error.code == NSURLErrorFileDoesNotExist ? downloader::DownloadStatus::FileNotFound
                                                       : downloader::DownloadStatus::Failed;
    }
    
    ASSERT(location, ());
    
    if (!m_queue.Contains(countryId))
      return;

    if (status == downloader::DownloadStatus::Failed && urls.size() > 1)
    {
      urls.pop_back();
      DownloadFromAnyUrl(countryId, downloadPath, urls);
    }
    else
    {
      m_queue.GetCountryById(countryId).OnDownloadFinished(status);
      m_queue.Remove(countryId);
    }
  };
  
  auto onProgress = [this, countryId](int64_t totalWritten, int64_t totalExpected) {
    if (!m_queue.Contains(countryId))
      return;

    auto const & country = m_queue.GetCountryById(countryId);
    country.OnDownloadProgress({totalWritten, totalExpected});
  };
  
  NSURL * url = [NSURL URLWithString:@(urls.back().c_str())];
  BackgroundDownloader * downloader = [BackgroundDownloader sharedBackgroundMapDownloader];
  NSUInteger taskId = [downloader downloadWithUrl:url completion:onFinish progress:onProgress];

  m_queue.SetTaskIdForCountryId(countryId, taskId);
}

std::unique_ptr<MapFilesDownloader> GetDownloader()
{
  return std::make_unique<BackgroundDownloaderAdapter>();
}
}  // namespace storage
