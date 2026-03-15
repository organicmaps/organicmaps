#include "platform/http_client_qt.hpp"

#include "base/logging.hpp"

#include <QMetaObject>

#include <QNetworkRequest>
#include <QPointer>
#include <QThread>
#include <QUrl>

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
}  // namespace

namespace platform
{
HttpClientReply::HttpClientReply(QNetworkReply * reply, HttpClient::CompletionHandler handler,
                                 HttpClient::ProgressHandler progressHandler, HttpClient::DataHandler dataHandler,
                                 CancelChecker cancelChecker, bool loadHeaders, bool followRedirects,
                                 std::string urlRequested, std::string cookies, std::string outputFile)
  : m_reply(reply)
  , m_handler(std::move(handler))
  , m_progressHandler(std::move(progressHandler))
  , m_dataHandler(std::move(dataHandler))
  , m_cancelChecker(std::move(cancelChecker))
  , m_loadHeaders(loadHeaders)
  , m_followRedirects(followRedirects)
  , m_urlRequested(std::move(urlRequested))
  , m_cookies(std::move(cookies))
  , m_outputFile(std::move(outputFile))
{
  if (!m_outputFile.empty())
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
  connect(m_reply, &QNetworkReply::readyRead, this, &HttpClientReply::OnReadyRead);
  connect(m_reply, &QNetworkReply::downloadProgress, this, &HttpClientReply::OnDownloadProgress);
  connect(m_reply, &QNetworkReply::finished, this, &HttpClientReply::OnFinished);

  // Abort immediately so we don't waste bandwidth on a doomed download.
  // Must be after signal connections so OnFinished() still fires.
  // Note: abort() triggers OnFinished() synchronously via direct connection,
  // but all members are already initialized at this point and OnFinished()
  // uses only deleteLater() for cleanup, so this is safe.
  if (m_writeError)
    m_reply->abort();
}

void HttpClientReply::OnReadyRead()
{
  if (m_writeError)
    return;

  QByteArray const data = m_reply->readAll();
  if (data.isEmpty())
    return;

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
    if (m_outputFileStream->write(data) != data.size())
    {
      LOG(LWARNING, ("File write error for:", m_outputFile));
      m_writeError = true;
      m_reply->abort();
      return;
    }
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
      if (m_outputFileStream)
      {
        QByteArray const remaining = m_reply->readAll();
        if (!remaining.isEmpty())
        {
          if (m_outputFileStream->write(remaining) != remaining.size())
            m_writeError = true;
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

  // File open or write error — report failure regardless of HTTP result.
  // Must be outside the if/else above because abort() on file-open failure
  // yields noError=false and httpCode=0, skipping the if branch entirely.
  if (m_writeError)
  {
    if (m_outputFileStream)
      m_outputFileStream->close();
    result.m_success = false;
    result.m_errorCode = HttpClient::kWriteException;
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
  auto impl = handle.m_impl;
  auto cancelChecker = handle.MakeCancelChecker();

  // Post to the network worker thread. The lambda runs on that thread
  // where QNetworkAccessManager lives, so QNetworkReply is created with
  // correct affinity and signals fire on the worker's event loop.
  auto & nt = GetNetworkThread();
  QMetaObject::invokeMethod(nt.worker,
                            [=, handler = std::move(handler), cancelChecker = std::move(cancelChecker)]() mutable
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
    QNetworkReply * reply = nullptr;

    if (httpMethod == "GET")
      reply = manager.get(request);
    else if (httpMethod == "POST")
      reply = manager.post(request, bodyBytes);
    else if (httpMethod == "PUT")
      reply = manager.put(request, bodyBytes);
    else if (httpMethod == "HEAD")
      reply = manager.head(request);
    else
      // sendCustomRequest supports body for any method, including DELETE.
      // QNetworkAccessManager::deleteResource() silently drops the body.
      reply = manager.sendCustomRequest(request, QByteArray::fromStdString(httpMethod), bodyBytes);

    new HttpClientReply(reply, std::move(handler), progressHandler, dataHandler, std::move(cancelChecker), loadHeaders,
                        followRedirects, urlRequested, cookies, outputFile);

    // Install platform cancel hook. reply->abort() must be called on the
    // reply's thread; use QMetaObject::invokeMethod with QueuedConnection.
    QPointer<QNetworkReply> guardedReply(reply);
    {
      std::lock_guard lock(impl->m_mu);
      impl->m_platformCancel = [guardedReply]
      {
        if (guardedReply)
          QMetaObject::invokeMethod(guardedReply.data(), "abort", Qt::QueuedConnection);
      };
    }

    // Double-check: if Cancel() ran between creating the reply and installing the hook,
    // m_cancelled is set but m_platformCancel was not called. Abort now.
    // We're on the worker thread (same as the reply), so direct abort is safe.
    if (impl->m_cancelled.load(std::memory_order_acquire))
    {
      if (guardedReply)
        guardedReply->abort();
    }
  }, Qt::QueuedConnection);

  return handle;
}
}  // namespace platform
