#pragma once

#include "platform/http_client.hpp"

#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>

#include <cstdint>
#include <optional>

namespace platform
{
using CancelChecker = HttpClient::CancelChecker;

// True for OpenStreetMap hosts: production www.openstreetmap.org / api.openstreetmap.org and
// the dev server master.apis.dev.openstreetmap.org (any *.openstreetmap.org subdomain plus the bare apex).
bool IsOsmHost(QString const & host);

// QObject helper that bridges Qt signals to HttpClient::CompletionHandler.
// Lives on the network thread, destroyed when the reply finishes.
class HttpClientReply : public QObject
{
  Q_OBJECT

public:
  // Installs the platform cancel hook against the given reply. Owns a captured
  // shared_ptr to the request handle's Impl, so it can be re-bound to a fresh
  // reply across a transparent retry without exposing Impl outside HttpClient.
  using RebindCancel = std::function<void(QNetworkReply *)>;

  HttpClientReply(QNetworkReply * reply, HttpClient::CompletionHandler handler,
                  HttpClient::ProgressHandler progressHandler, HttpClient::DataHandler dataHandler,
                  CancelChecker cancelChecker, bool loadHeaders, bool followRedirects, std::string urlRequested,
                  std::string cookies, std::string outputFile, std::optional<HttpClient::ReceivedFileSegment> segment,
                  QNetworkRequest request, std::string httpMethod, QByteArray bodyBytes, RebindCancel rebindCancel);

  // Pure decision predicate for the GOAWAY/REFUSED_STREAM retry. Exposed for
  // unit testing — cancellation is intentionally not part of this, it's an
  // orthogonal concern handled by the caller.
  static bool ShouldRetry(QNetworkReply::NetworkError error, QString const & errorString, int httpCode,
                          bool anyBytesReceived, bool writeError, int retriesRemaining);

private slots:
  void OnReadyRead();
  void OnDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
  void OnFinished();

private:
  // Validates 206 + Content-Range for segment mode and opens the output file at the target
  // offset on the first call. Idempotent; sets m_writeError + m_segmentErrorCode on failure.
  // Must be called before any write to the output file. Returns true if the segment is ready
  // to receive data, false if validation failed.
  bool ValidateSegmentIfNeeded();

  // Reused on initial attach and after a retry installs a fresh reply.
  void AttachReply(QNetworkReply * reply);

  // RFC 9113 §6.8: a stream above the GOAWAY's last-stream-id was provably not
  // processed by the server and is safe to retry. Re-issues once over HTTP/1.1
  // on a fresh connection. Returns true if a retry was scheduled (caller must
  // not touch m_reply afterwards — a new one is in flight).
  bool TryRetryOnGoAway();

  QNetworkReply * m_reply = nullptr;
  HttpClient::CompletionHandler m_handler;
  HttpClient::ProgressHandler m_progressHandler;
  HttpClient::DataHandler m_dataHandler;
  CancelChecker m_cancelChecker;
  bool m_loadHeaders;
  bool m_followRedirects;
  std::string m_urlRequested;
  std::string m_cookies;
  std::string m_outputFile;
  QFile * m_outputFileStream = nullptr;
  bool m_writeError = false;
  bool m_dataAborted = false;
  std::string m_accumulatedData;

  // Segment-mode state. When m_segment is set, m_outputFile is unused and the body is
  // streamed into m_segment->m_path at m_segment->m_offset after header validation.
  std::optional<HttpClient::ReceivedFileSegment> m_segment;
  bool m_segmentValidated = false;
  int64_t m_segmentBytesWritten = 0;
  int m_segmentErrorCode = HttpClient::kNoError;  // Overrides result error code when set.

  // Retry state: kept so a single GOAWAY-rejected stream can be re-issued
  // transparently. Flipped permanently when any body byte is observed by the
  // caller, so a retry can never silently double-deliver.
  QNetworkRequest m_request;
  std::string m_httpMethod;
  QByteArray m_bodyBytes;
  RebindCancel m_rebindCancel;
  int m_retriesRemaining = 1;
  bool m_anyBytesReceived = false;
};

// QObject worker living on a dedicated QThread. Owns the QNetworkAccessManager
// (created on the worker thread). All QNetworkReply objects and their signals
// live on this thread, so the caller's thread is never blocked for signal delivery.
class NetworkWorker : public QObject
{
  Q_OBJECT

public:
  // Parent = this ensures the manager is a child QObject and moves
  // together with the worker when moveToThread() is called.
  QNetworkAccessManager m_manager{this};
};
}  // namespace platform
