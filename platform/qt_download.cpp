#include "qt_download.hpp"
#include "qt_download_manager.hpp"

#include "../base/logging.hpp"
#include "../base/assert.hpp"

QtDownload::QtDownload(QtDownloadManager & manager, char const * url,
  char const * fileName, TDownloadFinishedFunction & finish, TDownloadProgressFunction & progress)
  : QObject(&manager), m_currentUrl(url), m_reply(0), m_file(0),
  m_httpRequestAborted(false), m_finish(finish), m_progress(progress)
{
  // use temporary file for download
  QString tmpFileName(fileName);
  tmpFileName += DOWNLOADING_FILE_EXTENSION;

  // @TODO implement resume download
  QFile::remove(tmpFileName);

  m_file = new QFile(tmpFileName);
  if (!m_file->open(QIODevice::WriteOnly))
  {
    QString const err = m_file->errorString();
    LOG(LERROR, ("Can't open file while downloading", qPrintable(tmpFileName), qPrintable(err)));
    delete m_file;
    m_file = 0;

    if (m_finish)
      m_finish(url, false);
    // mark itself to delete
    deleteLater();
    return;
  }

  // url acts as a key to find this download by QtDownloadManager::findChild(url)
  setObjectName(url);
  // this can be redirected later
  m_currentUrl = url;
  StartRequest();
}

QtDownload::~QtDownload()
{
  if (m_reply)
  {
    m_httpRequestAborted = true;
    // calls OnHttpFinished
    m_reply->abort();
  }
  LOG(LDEBUG, (qPrintable(objectName())));
}

void QtDownload::StartDownload(QtDownloadManager & manager, char const * url,
     char const * fileName, TDownloadFinishedFunction & finish, TDownloadProgressFunction & progress)
{
  ASSERT(url && fileName, ());
  // manager is responsible for auto deleting
  new QtDownload(manager, url, fileName, finish, progress);
}

void QtDownload::StartRequest()
{
  m_reply = static_cast<QtDownloadManager *>(parent())->NetAccessManager().get(QNetworkRequest(m_currentUrl));
  connect(m_reply, SIGNAL(finished()), this, SLOT(OnHttpFinished()));
  connect(m_reply, SIGNAL(readyRead()), this, SLOT(OnHttpReadyRead()));
  connect(m_reply, SIGNAL(downloadProgress(qint64, qint64)),
          this, SLOT(OnUpdateDataReadProgress(qint64, qint64)));
}

void QtDownload::OnHttpFinished()
{
  if (m_httpRequestAborted)
  { // we're called from destructor
    m_file->close();
    m_file->remove();
    delete m_file;
    m_file = 0;

    m_reply->deleteLater();
    m_reply = 0;

    // don't notify client when aborted
    //OnDownloadFinished(ToUtf8(m_originalUrl).c_str(), false, "Download was aborted");

    return;
  }

  m_file->flush();
  m_file->close();

  QVariant redirectionTarget = m_reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
  if (m_reply->error())
  {
    m_file->remove();
    delete m_file;
    m_file = 0;

    QString const err = m_reply->errorString();
    LOG(LWARNING, ("Download failed", qPrintable(err)));

    m_reply->deleteLater();
    m_reply = 0;

    if (m_finish)
      m_finish(objectName().toUtf8().data(), false);
    // selfdestruct
    deleteLater();
  }
  else if (!redirectionTarget.isNull())
  {
    m_currentUrl = m_currentUrl.resolved(redirectionTarget.toUrl());
    LOG(LINFO, ("HTTP redirect", m_currentUrl.toEncoded().data()));
    m_file->open(QIODevice::WriteOnly);
    m_file->resize(0);

    m_reply->deleteLater();
    StartRequest();
    return;
  }
  else
  { // download succeeded
    // original file name which was requested to download
    QString const originalFileName = m_file->fileName().left(m_file->fileName().lastIndexOf(DOWNLOADING_FILE_EXTENSION));
    bool resultForGui = true;
    // delete original file if it exists
    QFile::remove(originalFileName);
    if (!m_file->rename(originalFileName))
    { // sh*t... file is locked and can't be removed
      // report error to GUI
      LOG(LERROR, ("File exists and can't be replaced by downloaded one:", qPrintable(originalFileName)));
      resultForGui = false;
    }

    delete m_file;
    m_file = 0;
    m_reply->deleteLater();
    m_reply = 0;

    if (m_finish)
      m_finish(qPrintable(objectName()), resultForGui);
    // selfdestruct
    deleteLater();
  }
}

void QtDownload::OnHttpReadyRead()
{
  // this slot gets called everytime the QNetworkReply has new data.
  // We read all of its new data and write it into the file.
  // That way we use less RAM than when reading it at the finished()
  // signal of the QNetworkReply
  if (m_file && m_reply)
    m_file->write(m_reply->readAll());
}

void QtDownload::OnUpdateDataReadProgress(qint64 bytesRead, qint64 totalBytes)
{
  if (!m_httpRequestAborted && m_progress)
    m_progress(qPrintable(objectName()), TDownloadProgress(bytesRead, totalBytes));
}
