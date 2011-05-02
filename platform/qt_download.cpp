#include "qt_download.hpp"
#include "qt_download_manager.hpp"

#include "../base/logging.hpp"
#include "../base/assert.hpp"

#include "../coding/sha2.hpp"
#include "../coding/base64.hpp"

#include "../std/target_os.hpp"

#include <QNetworkInterface>
#include <QFSFileEngine>
#include <QDateTime>

// How many times we try to automatically reconnect in the case of network errors
#define MAX_AUTOMATIC_RETRIES 2

#ifdef OMIM_OS_WINDOWS
  #define LOGIN_VAR "USERNAME"
#else
  #define LOGIN_VAR "USER"
#endif
/// @return login name for active user and local machine hostname
static QString UserLoginAndHostname()
{
  char const * login = getenv(LOGIN_VAR);
  QString result(login ? login : "");
  result += QHostInfo::localHostName();
  return result;
}

/// @return mac address of active interface or empty string if not found
static QString MacAddress()
{
  QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
  for (int i = 0; i < interfaces.size(); ++i)
  {
    QNetworkInterface const & iface = interfaces.at(i);
    QString hwAddr = iface.hardwareAddress();
    if (!iface.addressEntries().empty()
      && (iface.flags() & (QNetworkInterface::IsUp | QNetworkInterface::IsRunning
      | QNetworkInterface::CanBroadcast | QNetworkInterface::CanMulticast)) == iface.flags())
    {
      return hwAddr;
    }
  }
  // no valid interface was found
  return QString();
}

/// @return creation time of the root file system or empty string
static QString FsCreationTime()
{
  QFileInfoList drives = QFSFileEngine::drives();
  for (int i = 0; i < drives.size(); ++i)
  {
    QFileInfo const & info = drives.at(i);
    QString const path = info.absolutePath();
    if (path == "/" || path.startsWith("C:"))
      return QString("%1").arg(info.created().toTime_t());
  }
  return QString();
}

static QString UniqueClientId()
{
  QString result = MacAddress();
  if (result.size() == 0)
  {
    result = FsCreationTime();
    if (result.size() == 0)
      result = QString("------------");
  }
  // add salt - login user name and local hostname
  result += UserLoginAndHostname();
  // calculate one-way hash
  QByteArray const original = QByteArray::fromHex(result.toLocal8Bit());
  string const hash = sha2::digest256(original.constData(), original.size(), false);
  // xor hash
  size_t const offset = hash.size() / 4;
  string xoredHash;
  for (size_t i = 0; i < offset; ++i)
    xoredHash.push_back(hash[i] ^ hash[i + offset] ^ hash[i + offset * 2] ^ hash[i + offset * 3]);
  return base64::encode(xoredHash).c_str();
}

static QString UserAgent()
{
  static QString userAgent = UniqueClientId();
  return userAgent;
}

QtDownload::QtDownload(QtDownloadManager & manager, HttpStartParams const & params)
  : QObject(&manager), m_currentUrl(params.m_url.c_str()), m_reply(0),
    m_retryCounter(0), m_params(params)
{
  // if we need to save server response to the file...
  if (!m_params.m_fileToSave.empty())
  {
    // use temporary file for download
    m_file.setFileName((m_params.m_fileToSave + DOWNLOADING_FILE_EXTENSION).c_str());
    if (!m_file.open(m_params.m_useResume ? QIODevice::Append : QIODevice::WriteOnly))
    {
      QString const err = m_file.errorString();
      LOG(LERROR, ("Can't open file while downloading", qPrintable(m_file.fileName()), qPrintable(err)));
      if (m_params.m_finish)
      {
        HttpFinishedParams result;
        result.m_error = EHttpDownloadCantCreateFile;
        result.m_file = m_params.m_fileToSave;
        result.m_url = m_params.m_url;
        m_params.m_finish(result);
      }
      // mark itself to delete
      deleteLater();
      return;
    }
  }
  // url acts as a key to find this download by QtDownloadManager::findChild(url)
  setObjectName(m_params.m_url.c_str());

  StartRequest();
}

QtDownload::~QtDownload()
{
  if (m_reply)
  {
    // don't notify client when aborted
    disconnect(m_reply, SIGNAL(finished()), this, SLOT(OnHttpFinished()));
    disconnect(m_reply, SIGNAL(readyRead()), this, SLOT(OnHttpReadyRead()));
    disconnect(m_reply, SIGNAL(downloadProgress(qint64, qint64)),
            this, SLOT(OnUpdateDataReadProgress(qint64, qint64)));
    m_reply->abort();
    delete m_reply;

    if (m_file.isOpen())
    {
      m_file.close();
      m_file.remove();
    }
  }
  LOG(LDEBUG, (m_params.m_url));
}

void QtDownload::StartRequest()
{
  QNetworkRequest httpRequest(m_currentUrl);
  // set user-agent with unique client id
  httpRequest.setRawHeader("User-Agent", UserAgent().toAscii());
  if (m_file.isOpen())
  {
    qint64 const fileSize = m_file.size();
    if (fileSize > 0) // need resume
      httpRequest.setRawHeader("Range", QString("bytes=%1-").arg(fileSize).toAscii());
  }
  if (!m_params.m_contentType.empty())
    httpRequest.setHeader(QNetworkRequest::ContentTypeHeader, m_params.m_contentType.c_str());
  if (m_params.m_postData.empty())
    m_reply = static_cast<QtDownloadManager *>(parent())->NetAccessManager().get(httpRequest);
  else
  {
    m_reply = static_cast<QtDownloadManager *>(parent())->NetAccessManager().post(
          httpRequest, m_params.m_postData.c_str());
  }
  connect(m_reply, SIGNAL(finished()), this, SLOT(OnHttpFinished()));
  connect(m_reply, SIGNAL(readyRead()), this, SLOT(OnHttpReadyRead()));
  connect(m_reply, SIGNAL(downloadProgress(qint64, qint64)),
          this, SLOT(OnUpdateDataReadProgress(qint64, qint64)));
}

static DownloadResultT TranslateError(QNetworkReply::NetworkError err)
{
  switch (err)
  {
  case QNetworkReply::NoError: return EHttpDownloadOk;
  case QNetworkReply::ContentNotFoundError: return EHttpDownloadFileNotFound;
  case QNetworkReply::HostNotFoundError: return EHttpDownloadFailed;
  default: return EHttpDownloadFailed;
  }
}

void QtDownload::OnHttpFinished()
{
  QVariant const redirectionTarget = m_reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
  QNetworkReply::NetworkError const netError = m_reply->error();
  if (netError)
  {
    if (netError <= QNetworkReply::UnknownNetworkError && ++m_retryCounter <= MAX_AUTOMATIC_RETRIES)
    { // try one more time
      m_reply->deleteLater();
      StartRequest();
      return;
    }
    // do not delete file if we can resume it's downloading later
    if (m_file.isOpen())
    {
      m_file.close();
      if (m_file.size() == 0)
        m_file.remove();
    }

    QString const err = m_reply->errorString();
    LOG(LWARNING, ("Download failed", qPrintable(err)));

    m_reply->deleteLater();
    m_reply = 0;

    if (m_params.m_finish)
    {
      HttpFinishedParams result;
      result.m_file = m_params.m_fileToSave;
      result.m_url = m_params.m_url;
      result.m_error = TranslateError(netError);
      m_params.m_finish(result);
    }
    // selfdestruct
    deleteLater();
  }
  else if (!redirectionTarget.isNull())
  {
    m_currentUrl = m_currentUrl.resolved(redirectionTarget.toUrl());
    LOG(LINFO, ("HTTP redirect", m_currentUrl.toEncoded().data()));
    if (m_file.isOpen())
    {
      m_file.close();
      m_file.open(QIODevice::WriteOnly);
      m_file.resize(0);
    }

    m_reply->deleteLater();
    m_reply = 0;
    StartRequest();
    return;
  }
  else
  { // download succeeded
    bool fileIsLocked = false;
    if (m_file.isOpen())
    {
      m_file.close();
      // delete original file if it exists
      QFile::remove(m_params.m_fileToSave.c_str());
      if (!m_file.rename(m_params.m_fileToSave.c_str()))
      { // sh*t... file is locked and can't be removed
        m_file.remove();
        // report error to GUI
        LOG(LWARNING, ("File exists and can't be replaced by downloaded one:", m_params.m_fileToSave));
        fileIsLocked = true;
      }
    }

    if (m_params.m_finish)
    {
      HttpFinishedParams result;
      result.m_url = m_params.m_url;
      result.m_file = m_params.m_fileToSave;
      QByteArray data = m_reply->readAll();
      result.m_data.assign(data.constData(), data.size());
      result.m_error = fileIsLocked ? EHttpDownloadFileIsLocked : EHttpDownloadOk;
      m_params.m_finish(result);
    }

    m_reply->deleteLater();
    m_reply = 0;
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
  if (m_file.isOpen() && m_reply)
    m_file.write(m_reply->readAll());

  // @note that for requests where m_file is not used, data accumulates
  // and m_reply->readAll() should be called in finish() slot handler
}

void QtDownload::OnUpdateDataReadProgress(qint64 bytesRead, qint64 totalBytes)
{
  if (m_params.m_progress)
  {
    HttpProgressT p;
    p.m_current = bytesRead;
    p.m_total = totalBytes;
    p.m_url = m_params.m_url;
    m_params.m_progress(p);
  }
}
