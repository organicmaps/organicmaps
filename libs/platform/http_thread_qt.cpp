#include "platform/http_thread_qt.hpp"

#include "platform/http_thread_callback.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QSslError>
#include <QUrl>

HttpThread::HttpThread(std::string const & url, downloader::IHttpThreadCallback & cb, int64_t beg, int64_t end,
                       int64_t size, std::string const & pb)
  : m_callback(cb)
  , m_begRange(beg)
  , m_endRange(end)
  , m_downloadedBytes(0)
  , m_expectedSize(size)
{
  QUrl const qUrl(url.c_str());
  QNetworkRequest request(qUrl);

  // use Range header only if we don't download whole file from start
  if (!(beg == 0 && end < 0))
  {
    if (end > 0)
    {
      LOG(LDEBUG, (url, "downloading range [", beg, ",", end, "]"));
      QString const range = QString("bytes=") + QString::number(beg) + '-' + QString::number(end);
      request.setRawHeader("Range", range.toUtf8());
    }
    else
    {
      LOG(LDEBUG, (url, "resuming download from position", beg));
      QString const range = QString("bytes=") + QString::number(beg) + '-';
      request.setRawHeader("Range", range.toUtf8());
    }
  }

  /// Use single instance for whole app
  static QNetworkAccessManager netManager;

  if (pb.empty())
    m_reply = netManager.get(request);
  else
  {
    request.setRawHeader("Content-Type", "application/json");
    request.setRawHeader("Content-Length", QString::number(pb.size()).toLocal8Bit());
    m_reply = netManager.post(request, pb.c_str());
  }

  connect(m_reply, SIGNAL(metaDataChanged()), this, SLOT(OnHeadersReceived()));
  connect(m_reply, SIGNAL(readyRead()), this, SLOT(OnChunkDownloaded()));
  connect(m_reply, SIGNAL(finished()), this, SLOT(OnDownloadFinished()));
  LOG(LDEBUG, ("Connecting to", url, "[", beg, ",", end, "]", "size=", size));
}

HttpThread::~HttpThread()
{
  LOG(LDEBUG, ("Destroying HttpThread"));
  disconnect(m_reply, SIGNAL(metaDataChanged()), this, SLOT(OnHeadersReceived()));
  disconnect(m_reply, SIGNAL(readyRead()), this, SLOT(OnChunkDownloaded()));
  disconnect(m_reply, SIGNAL(finished()), this, SLOT(OnDownloadFinished()));

  m_reply->deleteLater();
}

void HttpThread::OnHeadersReceived()
{
  // We don't notify our callback here, because OnDownloadFinished() will always be called
  // Note: after calling reply->abort() all other reply's members calls crash the app
  if (m_reply->error())
    return;

  int const httpStatusCode = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  // When we didn't ask for chunks, code should be 200
  // When we asked for a chunk, code should be 206
  bool const isChunk = !(m_begRange == 0 && m_endRange < 0);
  if ((isChunk && httpStatusCode != 206) || (!isChunk && httpStatusCode != 200))
  {
    LOG(LWARNING,
        ("Http request to", m_reply->url().toEncoded().constData(), "aborted with HTTP code", httpStatusCode));
    m_reply->abort();
  }
  else if (m_expectedSize > 0)
  {
    // try to get content length from Content-Range header first
    if (m_reply->hasRawHeader("Content-Range"))
    {
      QList<QByteArray> const contentRange = m_reply->rawHeader("Content-Range").split('/');
      int const numElements = contentRange.size();
      if (numElements && contentRange.at(numElements - 1).toLongLong() != m_expectedSize)
      {
        LOG(LWARNING, ("Http request to", m_reply->url().toEncoded().constData(),
                       "aborted - invalid Content-Range:", contentRange.at(numElements - 1).toLongLong()));
        m_reply->abort();
      }
    }
    else if (m_reply->hasRawHeader("Content-Length"))
    {
      QByteArray const header = m_reply->rawHeader("Content-Length");
      int64_t const expSize = header.toLongLong();
      if (expSize != m_expectedSize)
      {
        LOG(LWARNING, ("Http request to", m_reply->url().toEncoded().constData(),
                       "aborted - invalid Content-Length:", m_reply->rawHeader("Content-Length").toLongLong()));
        m_reply->abort();
      }
    }
    else
    {
      LOG(LWARNING, ("Http request to", m_reply->url().toEncoded().constData(),
                     "aborted, server didn't send any valid file size"));
      m_reply->abort();
    }
  }
}

void HttpThread::OnChunkDownloaded()
{
  QByteArray const data = m_reply->readAll();
  int const chunkSize = data.size();
  m_downloadedBytes += chunkSize;
  m_callback.OnWrite(m_begRange + m_downloadedBytes - chunkSize, data.constData(), chunkSize);
}

void HttpThread::OnDownloadFinished()
{
  if (m_reply->error() != QNetworkReply::NetworkError::NoError)
  {
    auto const httpStatusCode = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    LOG(LWARNING, ("Download has finished with code:", httpStatusCode, "error:", m_reply->errorString().toStdString()));
    m_callback.OnFinish(httpStatusCode, m_begRange, m_endRange);
  }
  else
  {
    m_callback.OnFinish(200, m_begRange, m_endRange);
  }
}

namespace downloader
{

HttpThread * CreateNativeHttpThread(std::string const & url, downloader::IHttpThreadCallback & cb, int64_t beg,
                                    int64_t end, int64_t size, std::string const & pb)
{
  return new HttpThread(url, cb, beg, end, size, pb);
}

void DeleteNativeHttpThread(HttpThread * request)
{
  delete request;
}
}  // namespace downloader
