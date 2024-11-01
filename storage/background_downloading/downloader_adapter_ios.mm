#import "storage/background_downloading/downloader_adapter_ios.h"

#import "network/internal/native/apple/background_downloader.h"

#include "storage/downloader.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"

#include <memory>
#include <utility>

@interface NSError (ToDownloaderError)

- (om::network::DownloadStatus)toDownloaderError;

@end

@implementation NSError (ToDownloaderError)

- (om::network::DownloadStatus)toDownloaderError {
  return self.code == NSURLErrorFileDoesNotExist ? om::network::DownloadStatus::FileNotFound
                                                 : om::network::DownloadStatus::Failed;
}

@end

namespace storage
{

void BackgroundDownloaderAdapter::Remove(CountryId const & countryId)
{
  MapFilesDownloader::Remove(countryId);
  
  if (!m_queue.Contains(countryId))
    return;

  BackgroundDownloader * downloader = [BackgroundDownloader sharedBackgroundMapDownloader];
  auto const taskIdentifier = m_queue.GetTaskInfoForCountryId(countryId);
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
}

QueueInterface const & BackgroundDownloaderAdapter::GetQueue() const
{
  if (m_queue.IsEmpty())
    return MapFilesDownloader::GetQueue();
    
  return m_queue;
}

void BackgroundDownloaderAdapter::Download(QueuedCountry && queuedCountry)
{
  if (!IsDownloadingAllowed())
  {
    queuedCountry.OnDownloadFinished(om::network::DownloadStatus::Failed);
    return;
  }

  auto const countryId = queuedCountry.GetCountryId();
  auto urls = MakeUrlList(queuedCountry.GetRelativeUrl());
  // Get urls order from worst to best.
  std::reverse(urls.begin(), urls.end());
  auto const path = queuedCountry.GetFileDownloadPath();

  // The order is important here: add to the queue first, notify start downloading then.
  // Infinite recursion possible here, otherwise:
  // OnStartDownloading -> NotifyStatusChanged -> processCountryEvent -> configDialog (?!)
  // -> downloadNode for the same country if autodownload enabled -> Download.
  m_queue.Append(QueuedCountry(queuedCountry));

  queuedCountry.OnStartDownloading();

  DownloadFromLastUrl(countryId, path, std::move(urls));
}

void BackgroundDownloaderAdapter::DownloadFromLastUrl(CountryId const & countryId,
                                                      std::string const & downloadPath,
                                                      std::vector<std::string> && urls)
{
  if (urls.empty())
    return;

  NSURL * url = [NSURL URLWithString:@(urls.back().c_str())];
  assert(url != nil);
  urls.pop_back();

  auto onFinish = [this, countryId, downloadPath, urls = std::move(urls)](NSError *error) mutable
  {
    om::network::DownloadStatus status = error ? [error toDownloaderError] : om::network::DownloadStatus::Completed;

    if (!m_queue.Contains(countryId))
      return;

    if (status == om::network::DownloadStatus::Failed && !urls.empty())
    {
      DownloadFromLastUrl(countryId, downloadPath, std::move(urls));
    }
    else
    {
      auto const country = m_queue.GetCountryById(countryId);
      m_queue.Remove(countryId);
      country.OnDownloadFinished(status);
    }
  };

  auto onProgress = [this, countryId](int64_t totalWritten, int64_t totalExpected)
  {
    if (!m_queue.Contains(countryId))
      return;

    auto const & country = m_queue.GetCountryById(countryId);
    country.OnDownloadProgress({totalWritten, totalExpected});
  };

  BackgroundDownloader * downloader = [BackgroundDownloader sharedBackgroundMapDownloader];
  NSUInteger taskId = [downloader downloadWithUrl:url completion:onFinish progress:onProgress];

  m_queue.SetTaskInfoForCountryId(countryId, taskId);
}

std::unique_ptr<MapFilesDownloader> GetDownloader()
{
  return std::make_unique<BackgroundDownloaderAdapter>();
}

}  // namespace storage
