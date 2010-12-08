#include "qt_download_manager.hpp"
#include "qt_download.hpp"

#include "../base/assert.hpp"

void QtDownloadManager::DownloadFile(char const * url, char const * fileName,
     TDownloadFinishedFunction finish, TDownloadProgressFunction progress,
     bool useResume)
{
  QList<QtDownload *> downloads = findChildren<QtDownload *>(url);
  if (downloads.empty())
    QtDownload::StartDownload(*this, url, fileName, finish, progress, useResume);
  else
  {
    ASSERT(false, ("Download with the same url is already in progress!"));
  }
}

void QtDownloadManager::CancelDownload(char const * url)
{
  QList<QtDownload *> downloads = findChildren<QtDownload *>(url);
  while (!downloads.isEmpty())
    delete downloads.takeFirst();
}

void QtDownloadManager::CancelAllDownloads()
{
  QList<QtDownload *> downloads = findChildren<QtDownload *>();
  while (!downloads.isEmpty())
    delete downloads.takeFirst();
}

extern "C" DownloadManager & GetDownloadManager()
{
  static QtDownloadManager dlMgr;
  return dlMgr;
}
