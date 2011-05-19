#include "qt_download_manager.hpp"
#include "qt_download.hpp"

#include "../base/assert.hpp"
#include "../base/logging.hpp"

void QtDownloadManager::CreateOnMainThreadSlot(HttpStartParams const & params)
{
  // manager is responsible for auto deleting
  new QtDownload(*this, params);
}

void QtDownloadManager::HttpRequest(HttpStartParams const & params)
{
  ASSERT(!params.m_url.empty(), ());
  QList<QtDownload *> downloads = findChildren<QtDownload *>(params.m_url.c_str());
  if (downloads.empty())
  {
    // small trick so all connections will be created on the main thread
    qRegisterMetaType<HttpStartParams>("HttpStartParams");
    QMetaObject::invokeMethod(this, "CreateOnMainThreadSlot", Q_ARG(HttpStartParams const & , params));
  }
  else
  {
    LOG(LWARNING, ("Download with the same url is already in progress, ignored", params.m_url));
  }
}

void QtDownloadManager::CancelDownload(string const & url)
{
  QList<QtDownload *> downloads = findChildren<QtDownload *>(url.c_str());
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
