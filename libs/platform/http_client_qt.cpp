#include "platform/http_client_qt.hpp"

#include "base/logging.hpp"

#include <QMetaObject>

#include <QNetworkRequest>
#include <QPointer>
#include <QThread>
#include <QUrl>
#include <QtGlobal>  // QT_VERSION, QT_VERSION_CHECK

namespace
{
// Dedicated network thread + worker. The NetworkWorker and its QNetworkAccessManager
// live on the thread, so all QNetworkReply signals fire on its event loop.
struct NetworkThread
{
  QThread thread;
  platform::NetworkWorker * worker = nullptr;

  NetworkThread()
  {
    // Create the worker here, then move it to the thread before starting.
    // moveToThread() is safe for objects that haven't received any events yet.
    // QNetworkAccessManager (as a child of worker) moves along with it.
    worker = new platform::NetworkWorker;
    worker->moveToThread(&thread);
    thread.start();
  }

  ~NetworkThread()
  {
    thread.quit();
    thread.wait();
    delete worker;
  }
};

NetworkThread & GetNetworkThread()
{
  static NetworkThread nt;
  return nt;
}

// Extract Set-Cookie values from a QNetworkReply using raw headers.
// Qt's typed SetCookieHeader (RFC 6265 parser) drops cookies from comma-joined values
// because RFC 6265 doesn't use commas as separators. NormalizeServerCookies handles both
// comma-joined and separate Set-Cookie headers correctly.
std::string CollectServerCookies(QNetworkReply * reply)
{
  std::string rawCookies;
  for (auto const & header : reply->rawHeaderPairs())
  {
    if (header.first.toLower() != "set-cookie")
      continue;
    if (!rawCookies.empty())
      rawCookies += ", ";
    rawCookies += header.second.toStdString();
  }
  return rawCookies.empty() ? std::string{} : platform::HttpClient::NormalizeServerCookies(std::move(rawCookies));
}

// sendCustomRequest supports body for any method, including DELETE.
// QNetworkAccessManager::deleteResource() silently drops the body.
QNetworkReply * IssueRequest(QNetworkAccessManager & manager, std::string const & method,
                             QNetworkRequest const & request, QByteArray const & body)
{
  if (method == "GET")
    return manager.get(request);
  if (method == "POST")
    return manager.post(request, body);
  if (method == "PUT")
    return manager.put(request, body);
  if (method == "HEAD")
    return manager.head(request);
  return manager.sendCustomRequest(request, QByteArray::fromStdString(method), body);
}
}  // namespace

namespace platform
{
bool IsOsmHost(QString const & host)
{
  return host.compare(QLatin1String("openstreetmap.org"), Qt::CaseInsensitive) == 0 ||
         host.endsWith(QLatin1String(".openstreetmap.org"), Qt::CaseInsensitive);
}

HttpClientReply::HttpClientReply(QNetworkReply * reply, HttpClient::CompletionHandler handler,
                                 HttpClient::ProgressHandler progressHandler, HttpClient::DataHandler dataHandler,
                                 CancelChecker cancelChecker, bool loadHeaders, bool followRedirects,
                                 std::string urlRequested, std::string cookies, std::string outputFile,
                                 std::optional<HttpClient::ReceivedFileSegment> segment, QNetworkRequest request,
                                 std::string httpMethod, QByteArray bodyBytes, RebindCancel rebindCancel)
  : m_handler(std::move(handler))
  , m_progressHandler(std::move(progressHandler))
  , m_dataHandler(std::move(dataHandler))
  , m_cancelChecker(std::move(cancelChecker))
  , m_loadHeaders(loadHeaders)
  , m_followRedirects(followRedirects)
  , m_urlRequested(std::move(urlRequested))
  , m_cookies(std::move(cookies))
  , m_outputFile(std::move(outputFile))
  , m_segment(std::move(segment))
  , m_request(std::move(request))
  , m_httpMethod(std::move(httpMethod))
  , m_bodyBytes(std::move(bodyBytes))
  , m_rebindCancel(std::move(rebindCancel))
{
  // Segment mode takes precedence over plain output file and defers file open
  // until 206 + Content-Range are validated in ValidateSegmentIfNeeded().
  if (!m_segment && !m_outputFile.empty())
  {
    m_outputFileStream = new QFile(QString::fromStdString(m_outputFile), this);
    if (!m_outputFileStream->open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
      LOG(LWARNING, ("Can't open output file for writing:", m_outputFile));
      delete m_outputFileStream;
      m_outputFileStream = nullptr;
      m_writeError = true;
    }
  }
  AttachReply(reply);

  // Abort immediately so we don't waste bandwidth on a doomed download.
  // Must be after AttachReply so OnFinished() still fires.
  // Note: abort() triggers OnFinished() synchronously via direct connection,
  // but all members are already initialized at this point and OnFinished()
  // uses only deleteLater() for cleanup, so this is safe.
  if (m_writeError)
    m_reply->abort();
}

void HttpClientReply::AttachReply(QNetworkReply * reply)
{
  m_reply = reply;
  connect(reply, &QNetworkReply::readyRead, this, &HttpClientReply::OnReadyRead);
  connect(reply, &QNetworkReply::downloadProgress, this, &HttpClientReply::OnDownloadProgress);
  connect(reply, &QNetworkReply::finished, this, &HttpClientReply::OnFinished);

  // Re-point the platform cancel hook at the new reply. Cancel() called between
  // the old hook firing (no-op against a destroyed reply) and the rebind would
  // have set m_cancelled, so check the cancel-checker afterwards and abort now.
  m_rebindCancel(reply);
  if (m_cancelChecker && m_cancelChecker())
    reply->abort();
}

// static
bool HttpClientReply::ShouldRetry(QNetworkReply::NetworkError error, QString const & errorString, int httpCode,
                                  bool anyBytesReceived, bool writeError, int retriesRemaining)
{
  if (writeError || anyBytesReceived || retriesRemaining <= 0)
    return false;
  // Any HTTP status means the server processed (at least the headers of) this request.
  if (httpCode != 0)
    return false;

  switch (error)
  {
  case QNetworkReply::ContentReSendError: return true;

  case QNetworkReply::ProtocolFailure:
    // Defensive secondary signal: Qt 6.x's HTTP/2 layer logs these phrases when a
    // stream is rejected after GOAWAY or with REFUSED_STREAM (see Qt sources
    // qhttp2connection.cpp / qhttpnetworkconnectionchannel.cpp — not public API,
    // re-verify on Qt upgrade). RFC 9113 §6.8 permits retrying these on a new
    // connection.
    return errorString.contains("stopped accepting", Qt::CaseInsensitive) ||
           errorString.contains("REFUSED_STREAM", Qt::CaseInsensitive) ||
           errorString.contains("GOAWAY", Qt::CaseInsensitive);

  default: return false;
  }
}

bool HttpClientReply::TryRetryOnGoAway()
{
  if (m_cancelChecker && m_cancelChecker())
    return false;

  int const httpCode = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  if (!ShouldRetry(m_reply->error(), m_reply->errorString(), httpCode, m_anyBytesReceived, m_writeError,
                   m_retriesRemaining))
    return false;

  --m_retriesRemaining;
  LOG(LWARNING, ("HTTP/2 stream rejected, retrying once over HTTP/1.1:", m_urlRequested,
                 "Qt error:", static_cast<int>(m_reply->error()), m_reply->errorString().toStdString()));

  m_reply->disconnect(this);
  m_reply->deleteLater();

  // Force HTTP/1.1 for the retry so we cannot multiplex onto another HTTP/2
  // connection from the same poisoned pool. The GOAWAY'd HTTP/2 connection is
  // already closed by the server; Qt will open a fresh TCP/TLS leg naturally.
  m_request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);

  auto & manager = GetNetworkThread().worker->m_manager;
  AttachReply(IssueRequest(manager, m_httpMethod, m_request, m_bodyBytes));
  return true;
}

bool HttpClientReply::ValidateSegmentIfNeeded()
{
  if (!m_segment)
    return !m_writeError;
  if (m_segmentValidated)
    return !m_writeError;
  // Only run once, even on failure.
  m_segmentValidated = true;

  int const httpCode = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  QByteArray const crBytes = m_reply->rawHeader("Content-Range");
  std::string_view const contentRange = crBytes.isEmpty()
                                          ? std::string_view()
                                          : std::string_view(crBytes.constData(), static_cast<size_t>(crBytes.size()));
  auto const validation = HttpClient::ValidateReceivedFileSegmentResponse(httpCode, contentRange, *m_segment);
  if (!validation.m_ok)
  {
    m_writeError = true;
    m_segmentErrorCode = validation.m_errorCode;
    return false;
  }

  // Open the existing target file without truncating and seek to the segment offset.
  m_outputFileStream = new QFile(QString::fromStdString(m_segment->m_path), this);
  if (!m_outputFileStream->open(QIODevice::ReadWrite | QIODevice::ExistingOnly))
  {
    LOG(LWARNING, ("Can't open segment file for writing:", m_segment->m_path));
    delete m_outputFileStream;
    m_outputFileStream = nullptr;
    m_writeError = true;
    m_segmentErrorCode = HttpClient::kWriteException;
    return false;
  }
  if (!m_outputFileStream->seek(static_cast<qint64>(m_segment->m_offset)))
  {
    LOG(LWARNING, ("Can't seek segment file", m_segment->m_path, "to offset", m_segment->m_offset));
    m_outputFileStream->close();
    m_writeError = true;
    m_segmentErrorCode = HttpClient::kWriteException;
    return false;
  }
  return true;
}

void HttpClientReply::OnReadyRead()
{
  if (m_writeError)
    return;

  // Segment mode requires header validation BEFORE any byte is written to disk. If the
  // response is not a valid 206 Partial Content for the requested range, fail fast —
  // otherwise we'd overwrite adjacent chunks' data at the wrong offset.
  if (m_segment && !ValidateSegmentIfNeeded())
  {
    // Drain and discard; abort the transfer.
    m_reply->readAll();
    m_reply->abort();
    return;
  }

  QByteArray const data = m_reply->readAll();
  if (data.isEmpty())
    return;

  // Past this point the caller will observe body bytes; retry would double-deliver.
  m_anyBytesReceived = true;

  if (m_dataHandler)
  {
    if (!m_dataHandler(data.constData(), static_cast<size_t>(data.size())))
    {
      m_dataAborted = true;
      m_reply->abort();
      return;
    }
  }
  else if (m_outputFileStream)
  {
    // Segment mode: refuse to write past the expected segment length — the server is
    // sending more than advertised in Content-Range.
    if (m_segment)
    {
      int64_t const remaining = m_segment->m_expectedBytes - m_segmentBytesWritten;
      if (data.size() > remaining)
      {
        LOG(LWARNING, ("Segment overflow:", data.size(), "bytes received, only", remaining, "expected"));
        m_writeError = true;
        m_segmentErrorCode = HttpClient::kInconsistentFileSize;
        m_reply->abort();
        return;
      }
    }
    if (m_outputFileStream->write(data) != data.size())
    {
      LOG(LWARNING, ("File write error for:", m_segment ? m_segment->m_path : m_outputFile));
      m_writeError = true;
      if (m_segment)
        m_segmentErrorCode = HttpClient::kWriteException;
      m_reply->abort();
      return;
    }
    if (m_segment)
      m_segmentBytesWritten += data.size();
  }
  else
  {
    m_accumulatedData.append(data.constData(), static_cast<size_t>(data.size()));
  }
}

void HttpClientReply::OnDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
  if (m_writeError)
    return;

  if (m_progressHandler)
    m_progressHandler(static_cast<int64_t>(bytesReceived), static_cast<int64_t>(bytesTotal));
}

void HttpClientReply::OnFinished()
{
  HttpClient::Result result;

  // Check cancellation.
  if (m_cancelChecker && m_cancelChecker())
  {
    result.m_errorCode = HttpClient::kCancelled;
    if (m_handler)
      m_handler(std::move(result));
    m_reply->deleteLater();
    deleteLater();
    return;
  }

  // Transparent retry for HTTP/2 GOAWAY-rejected streams (RFC 9113 §6.8).
  // On success a new reply is in flight and will fire OnFinished again.
  if (TryRetryOnGoAway())
    return;

  int const httpCode = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  bool const noError = (m_reply->error() == QNetworkReply::NoError);

  if (noError || httpCode != 0)
  {
    result.m_errorCode = httpCode != 0 ? httpCode : static_cast<int>(m_reply->error());
    // Any HTTP response (even 4xx/5xx) means the connection was made successfully.
    // Qt maps HTTP errors to QNetworkReply error codes, but callers expect m_success = true
    // when they got an HTTP response (matching Apple/Android semantics).
    result.m_success = (httpCode != 0);

    // Use toEncoded() to preserve percent-encoding, matching Apple's absoluteString behavior.
    // toString() decodes %XX sequences, which breaks WasRedirected() URL comparison.
    QUrl const redirectUrl = m_reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
    if (redirectUrl.isValid())
      result.m_urlReceived = redirectUrl.toEncoded().toStdString();
    else
      result.m_urlReceived = m_reply->url().toEncoded().toStdString();

    std::string const serverCookies = CollectServerCookies(m_reply);

    if (m_loadHeaders)
    {
      // Join duplicate headers, but keep Set-Cookie on the typed path above.
      for (auto const & header : m_reply->rawHeaderPairs())
      {
        auto const key = header.first.toLower().toStdString();
        if (key == "set-cookie")
          continue;
        auto const value = header.second.toStdString();
        auto [it, inserted] = result.m_headers.emplace(key, value);
        if (!inserted)
          it->second += ", " + value;
      }
      if (!serverCookies.empty())
        result.m_headers["set-cookie"] = serverCookies;
    }
    else
    {
      if (!serverCookies.empty())
        result.m_headers.emplace("set-cookie", serverCookies);
    }

    if (!m_dataHandler)
    {
      // Segment mode: validate headers here too, in case the body arrived with no
      // readyRead firing (e.g. empty 206 body, or status != 206).
      if (m_segment)
        ValidateSegmentIfNeeded();

      if (m_outputFileStream)
      {
        QByteArray const remaining = m_reply->readAll();
        if (!remaining.isEmpty())
        {
          // Segment overflow check: any bytes past expectedBytes are a protocol violation.
          if (m_segment)
          {
            int64_t const available = m_segment->m_expectedBytes - m_segmentBytesWritten;
            if (remaining.size() > available)
            {
              LOG(LWARNING,
                  ("Segment overflow at finish:", remaining.size(), "bytes trailing, only", available, "expected"));
              m_writeError = true;
              m_segmentErrorCode = HttpClient::kInconsistentFileSize;
            }
          }
          if (!m_writeError && m_outputFileStream->write(remaining) != remaining.size())
          {
            m_writeError = true;
            if (m_segment)
              m_segmentErrorCode = HttpClient::kWriteException;
          }
          else if (!m_writeError && m_segment)
          {
            m_segmentBytesWritten += remaining.size();
          }
        }
        m_outputFileStream->close();
      }
      else
      {
        QByteArray const body = m_reply->readAll();
        if (!body.isEmpty())
          m_accumulatedData.append(body.constData(), static_cast<size_t>(body.size()));
        result.m_serverResponse = std::move(m_accumulatedData);
      }
    }
  }
  else
  {
    result.m_errorCode = static_cast<int>(m_reply->error());
    LOG(LDEBUG, ("Network error:", m_reply->errorString().toStdString(), "while connecting to", m_urlRequested));
  }

  // Segment-mode: verify the server delivered exactly the number of bytes advertised
  // by Content-Range. A short body means the transfer ended before the segment was
  // fully received, which must NOT be treated as a successful chunk.
  if (m_segment && !m_writeError && m_segmentValidated && m_segmentBytesWritten != m_segment->m_expectedBytes)
  {
    LOG(LWARNING, ("Segment underflow:", m_segmentBytesWritten, "bytes written, expected", m_segment->m_expectedBytes));
    m_writeError = true;
    m_segmentErrorCode = HttpClient::kInconsistentFileSize;
  }

  // File open or write error — report failure regardless of HTTP result.
  // Must be outside the if/else above because abort() on file-open failure
  // yields noError=false and httpCode=0, skipping the if branch entirely.
  if (m_writeError)
  {
    if (m_outputFileStream)
      m_outputFileStream->close();
    result.m_success = false;
    if (m_segmentErrorCode != HttpClient::kNoError)
      result.m_errorCode = m_segmentErrorCode;
    else if (!m_segment)
      result.m_errorCode = HttpClient::kWriteException;
    // Segment mode with m_segmentErrorCode == kNoError: preserve the HTTP code (e.g. 404),
    // so the downloader's 404 → FileNotFound mapping still works.
  }

  // DataHandler requested abort — override the HTTP success.
  if (m_dataAborted)
    result.m_success = false;

  if (m_handler)
    m_handler(std::move(result));

  m_reply->deleteLater();
  deleteLater();
}

HttpClient::RequestHandle HttpClient::RunHttpRequestAsync(CompletionHandler handler)
{
  RequestHandle handle;
  handle.m_impl = std::make_shared<RequestHandle::Impl>();

  QNetworkRequest request(QUrl(QString::fromStdString(m_urlRequested)));

  request.setTransferTimeout(static_cast<int>(m_timeoutSec * 1000));

#if QT_VERSION < QT_VERSION_CHECK(6, 5, 1)
  // QTBUG-111417: on Qt <= 6.4 an HTTP/2 QNetworkReply can stall without ever emitting
  // finished() until the peer closes the connection, hanging the blocking RunHttpRequest()
  // far past setTransferTimeout. OSM login/OAuth (www) and the editing API negotiate HTTP/2,
  // so force HTTP/1.1 for OSM hosts. Fixed in Qt 6.5.1/6.6.0 — this block compiles out on
  // newer Qt, where HTTP/2 is used again.
  if (IsOsmHost(request.url().host()))
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
#endif

  // Disable Qt's automatic cookie handling — we manage cookies manually via
  // SetCookies() / CombinedCookies(), matching Apple's HTTPShouldHandleCookies=NO.
  // Without this, QNetworkAccessManager's cookie jar may consume Set-Cookie headers
  // so they don't appear in rawHeaderPairs(), breaking cookie-dependent auth flows.
  request.setAttribute(QNetworkRequest::CookieLoadControlAttribute, QNetworkRequest::Manual);
  request.setAttribute(QNetworkRequest::CookieSaveControlAttribute, QNetworkRequest::Manual);

  for (auto const & header : m_headers)
    request.setRawHeader(QByteArray::fromStdString(header.first), QByteArray::fromStdString(header.second));

  if (!m_cookies.empty())
    request.setRawHeader("Cookie", QByteArray::fromStdString(m_cookies));

  if (m_followRedirects)
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
  else
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);

  // QNetworkAccessManager handles Accept-Encoding and transparent decompression automatically.

  QByteArray bodyBytes;
  if (!m_bodyData.empty())
  {
    bodyBytes = QByteArray(m_bodyData.data(), static_cast<int>(m_bodyData.size()));
  }
  else if (!m_inputFile.empty())
  {
    QFile file(QString::fromStdString(m_inputFile));
    if (file.open(QIODevice::ReadOnly))
    {
      bodyBytes = file.readAll();
    }
    else
    {
      LOG(LWARNING, ("Can't open input file for reading:", m_inputFile));
      if (handler)
        handler(Result{});
      return handle;
    }
  }

  // Capture all config for the lambda posted to the network thread.
  auto const httpMethod = m_httpMethod;
  auto const progressHandler = m_progressHandler;
  auto const dataHandler = m_dataHandler;
  auto const loadHeaders = m_loadHeaders;
  auto const followRedirects = m_followRedirects;
  auto const urlRequested = m_urlRequested;
  auto const cookies = m_cookies;
  auto const outputFile = m_outputFile;
  auto const segment = m_receivedFileSegment;
  auto impl = handle.m_impl;
  auto cancelChecker = handle.MakeCancelChecker();

  // reply->abort() must be invoked on the reply's thread, so the inner cancel
  // dispatches via QueuedConnection regardless of which thread Cancel() runs on.
  auto const rebindCancel = [impl](QNetworkReply * reply)
  {
    QPointer<QNetworkReply> guardedReply(reply);
    std::lock_guard lock(impl->m_mu);
    impl->m_platformCancel = [guardedReply]
    {
      if (guardedReply)
        QMetaObject::invokeMethod(guardedReply.data(), "abort", Qt::QueuedConnection);
    };
  };

  // Post to the network worker thread. The lambda runs on that thread
  // where QNetworkAccessManager lives, so QNetworkReply is created with
  // correct affinity and signals fire on the worker's event loop.
  auto & nt = GetNetworkThread();
  QMetaObject::invokeMethod(nt.worker,
                            [=, handler = std::move(handler), cancelChecker = std::move(cancelChecker),
                             rebindCancel = std::move(rebindCancel)]() mutable
  {
    // Check cancellation BEFORE creating the request — fixes the race where
    // Cancel() is called between RunHttpRequestAsync() returning and this
    // lambda executing on the network thread.
    if (cancelChecker())
    {
      HttpClient::Result result;
      result.m_errorCode = HttpClient::kCancelled;
      if (handler)
        handler(std::move(result));
      return;
    }

    auto & manager = GetNetworkThread().worker->m_manager;
    QNetworkReply * reply = IssueRequest(manager, httpMethod, request, bodyBytes);

    new HttpClientReply(reply, std::move(handler), progressHandler, dataHandler, std::move(cancelChecker), loadHeaders,
                        followRedirects, urlRequested, cookies, outputFile, segment, request, httpMethod, bodyBytes,
                        std::move(rebindCancel));
  },
                            Qt::QueuedConnection);

  return handle;
}
}  // namespace platform
