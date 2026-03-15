#pragma once

#include "platform/http_client.hpp"

#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>

namespace platform
{
using CancelChecker = HttpClient::CancelChecker;

// QObject helper that bridges Qt signals to HttpClient::CompletionHandler.
// Lives on the network thread, destroyed when the reply finishes.
class HttpClientReply : public QObject
{
  Q_OBJECT

public:
  HttpClientReply(QNetworkReply * reply, HttpClient::CompletionHandler handler,
                  HttpClient::ProgressHandler progressHandler, HttpClient::DataHandler dataHandler,
                  CancelChecker cancelChecker, bool loadHeaders, bool followRedirects, std::string urlRequested,
                  std::string cookies, std::string outputFile);

private slots:
  void OnReadyRead();
  void OnDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
  void OnFinished();

private:
  QNetworkReply * m_reply;
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
