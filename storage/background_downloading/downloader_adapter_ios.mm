#import "storage/background_downloading/downloader_adapter_ios.h"

#import "platform/background_downloader_ios.h"

#include "storage/downloader.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"

#include <memory>
#include <utility>

@interface NSError (ToDownloaderError)

- (downloader::DownloadStatus)toDownloaderError;

@end

@implementation NSError (ToDownloaderError)

- (downloader::DownloadStatus)toDownloaderError {
  return self.code == NSURLErrorFileDoesNotExist ? downloader::DownloadStatus::FileNotFound
                                                 : downloader::DownloadStatus::Failed;
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
    queuedCountry.OnDownloadFinished(downloader::DownloadStatus::Failed);
    return;
  }

  auto const countryId = queuedCountry.GetCountryId();
  auto const urls = MakeUrlList(queuedCountry.GetRelativeUrl());
  auto const path = queuedCountry.GetFileDownloadPath();

  m_queue.Append(std::move(queuedCountry));

  DownloadFromLastUrl(countryId, path, urls);
}

void BackgroundDownloaderAdapter::DownloadFromLastUrl(CountryId const & countryId,
                                                      std::string const & downloadPath,
                                                      std::vector<std::string> const & urls)
{
  if (urls.empty())
    return;

  auto onFinish = [this, countryId, downloadPath, urls = urls](NSError *error) mutable {
    downloader::DownloadStatus status = error ? [error toDownloaderError] : downloader::DownloadStatus::Completed;

    if (!m_queue.Contains(countryId))
      return;

    if (status == downloader::DownloadStatus::Failed && urls.size() > 1)
    {
      urls.pop_back();
      DownloadFromLastUrl(countryId, downloadPath, urls);
    }
    else
    {
      auto const country = m_queue.GetCountryById(countryId);
      m_queue.Remove(countryId);
      country.OnDownloadFinished(status);
    }
  };

  auto onProgress = [this, countryId](int64_t totalWritten, int64_t totalExpected) {
    if (!m_queue.Contains(countryId))
      return;

    auto const & country = m_queue.GetCountryById(countryId);
    country.OnDownloadProgress({totalWritten, totalExpected});
  };
  
  NSURL * url = [NSURL URLWithString:@(urls.back().c_str())];
  assert(url != nil);
  BackgroundDownloader * downloader = [BackgroundDownloader sharedBackgroundMapDownloader];
  NSUInteger taskId = [downloader downloadWithUrl:url completion:onFinish progress:onProgress];

  m_queue.SetTaskInfoForCountryId(countryId, taskId);
}

std::unique_ptr<MapFilesDownloader> GetDownloader()
{
  return std::make_unique<BackgroundDownloaderAdapter>();
}

}  // namespace storage
