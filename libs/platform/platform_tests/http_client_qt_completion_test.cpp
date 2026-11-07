#include "testing/testing.hpp"

#include "platform/http_client_qt.hpp"

#include <QCoreApplication>
#include <QEvent>
#include <QEventLoop>
#include <QHostAddress>
#include <QMetaObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryFile>

#include <algorithm>
#include <cstring>
#include <optional>

namespace http_client_qt_completion_test
{
using platform::HttpClient;

class Reply final : public QNetworkReply
{
public:
  explicit Reply(int status, std::optional<HttpClient::ReceivedFileSegment> const & segment)
  {
    setUrl(QUrl("http://localhost/completion"));
    setAttribute(QNetworkRequest::HttpStatusCodeAttribute, status);
    if (segment)
      setRawHeader("Content-Range", "bytes " + QByteArray::number(qint64(segment->m_offset)) + "-" +
                                        QByteArray::number(qint64(segment->m_offset + segment->m_expectedBytes - 1)) +
                                        "/" + QByteArray::number(qint64(segment->m_expectedTotalBytes)));
    open(QIODevice::ReadOnly);
  }

  void Deliver(QByteArray data, bool signalReady)
  {
    m_data = std::move(data);
    if (signalReady)
      emit readyRead();
  }

  void Finish(NetworkError error)
  {
    if (isFinished())
      return;
    setError(error, "Completion test");
    setFinished(true);
    emit finished();
  }

  void abort() override { Finish(OperationCanceledError); }

protected:
  qint64 readData(char * data, qint64 size) override
  {
    auto const count = std::min<qint64>(size, m_data.size());
    std::memcpy(data, m_data.constData(), static_cast<size_t>(count));
    m_data.remove(0, count);
    return count;
  }

private:
  QByteArray m_data;
};

struct CompletionOptions
{
  std::string m_path;
  std::optional<HttpClient::ReceivedFileSegment> m_segment;
  HttpClient::DataHandler m_dataHandler;
  bool m_signalReady = true;
  bool m_cancelled = false;
  QByteArray m_body = "abc";
  std::optional<QByteArray> m_fileContents;
};

HttpClient::Result Complete(int status, QNetworkReply::NetworkError error, CompletionOptions options = {})
{
  auto * reply = new Reply(status, options.m_segment);
  std::optional<HttpClient::Result> result;
  bool cancelRequested = false;
  platform::HttpClientReply * bridge = nullptr;
  bridge = new platform::HttpClientReply(reply,
                                         [&](HttpClient::Result value)
  {
    TEST(bridge, ("The fixture requires construction to finish before completion"));
    if (auto * file = bridge->findChild<QFile *>())
    {
      TEST(!file->isOpen(), ());
      if (options.m_fileContents)
      {
        QFile completed(file->fileName());
        TEST(completed.open(QIODevice::ReadOnly), ());
        TEST_EQUAL(completed.readAll().toStdString(), options.m_fileContents->toStdString(), ());
      }
    }
    else
      TEST(!options.m_fileContents, ("Expected an output file"));
    TEST(!result, ("Completion must run once"));
    result = std::move(value);
  }, {}, std::move(options.m_dataHandler), [&cancelRequested] { return cancelRequested; }, false,
                                         "http://localhost/completion", options.m_path, options.m_segment,
                                         QNetworkRequest(), "GET", {}, [](QNetworkReply *) {});
  reply->Deliver(options.m_body, options.m_signalReady);
  cancelRequested = options.m_cancelled;
  reply->Finish(error);
  TEST(result, ());
  QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
  return std::move(*result);
}

UNIT_TEST(HttpClientQt_CompletedHttpResponses)
{
  struct Case
  {
    int status;
    QNetworkReply::NetworkError error;
  };
  for (auto const [status, error] : {Case{200, QNetworkReply::NoError},
                                     {206, QNetworkReply::NoError},
                                     {301, QNetworkReply::NoError},
                                     {400, QNetworkReply::ProtocolInvalidOperationError},
                                     {401, QNetworkReply::AuthenticationRequiredError},
                                     {403, QNetworkReply::ContentAccessDenied},
                                     {404, QNetworkReply::ContentNotFoundError},
                                     {405, QNetworkReply::ContentOperationNotPermittedError},
                                     {407, QNetworkReply::ProxyAuthenticationRequiredError},
                                     {409, QNetworkReply::ContentConflictError},
                                     {410, QNetworkReply::ContentGoneError},
                                     {418, QNetworkReply::ProtocolInvalidOperationError},
                                     {429, QNetworkReply::UnknownContentError},
                                     {500, QNetworkReply::InternalServerError},
                                     {501, QNetworkReply::OperationNotImplementedError},
                                     {502, QNetworkReply::UnknownServerError},
                                     {503, QNetworkReply::ServiceUnavailableError}})
  {
    auto const result = Complete(status, error);
    TEST(result.m_success, (status, error));
    TEST_EQUAL(result.m_errorCode, status, ());
    TEST_EQUAL(result.m_serverResponse, "abc", ());
  }
}

UNIT_TEST(HttpClientQt_InterruptedResponses)
{
  for (int const status : {200, 206, 301, 404, 500})
  {
    for (auto const error :
         {QNetworkReply::TimeoutError, QNetworkReply::OperationCanceledError, QNetworkReply::RemoteHostClosedError,
          QNetworkReply::ProxyConnectionClosedError, QNetworkReply::ProtocolFailure, QNetworkReply::ContentReSendError})
    {
      auto const result = Complete(status, error);
      TEST(!result.m_success, (status, error));
      TEST_EQUAL(result.m_errorCode, static_cast<int>(error), ());
    }
  }
}

UNIT_TEST(HttpClientQt_InterruptedFileAndStreamingResponses)
{
  QTemporaryFile file;
  TEST(file.open(), ());
  auto const path = file.fileName().toStdString();
  file.close();
  auto const result = Complete(200, QNetworkReply::TimeoutError, {.m_path = path});
  TEST(!result.m_success, ());
  TEST_EQUAL(result.m_errorCode, static_cast<int>(QNetworkReply::TimeoutError), ());

  auto const cancelled = Complete(200, QNetworkReply::OperationCanceledError, {.m_path = path, .m_cancelled = true});
  TEST(!cancelled.m_success, ());
  TEST_EQUAL(cancelled.m_errorCode, HttpClient::kCancelled, ());

  std::string bytes;
  auto const streamed = Complete(200, QNetworkReply::RemoteHostClosedError,
                                 {.m_dataHandler = [&](void const * data, size_t size)
  {
    bytes.append(static_cast<char const *>(data), size);
    return true;
  }});
  TEST_EQUAL(bytes, "abc", ());
  TEST(!streamed.m_success, ());
  TEST_EQUAL(streamed.m_errorCode, static_cast<int>(QNetworkReply::RemoteHostClosedError), ());
}

UNIT_TEST(HttpClientQt_CompletedFilesAndSegments)
{
  for (bool const segment : {false, true})
  {
    for (bool const signalReady : {false, true})
    {
      for (bool const cancelled : {false, true})
      {
        QTemporaryFile file;
        TEST(file.open(), ());
        TEST_EQUAL(file.write("ABCDEFGH"), 8, ());
        auto const path = file.fileName().toStdString();
        file.close();
        CompletionOptions options{.m_path = path, .m_signalReady = signalReady, .m_cancelled = cancelled};
        if (segment)
          options.m_segment = HttpClient::ReceivedFileSegment{path, 2, 3, 8};
        if (signalReady || !cancelled)
          options.m_fileContents = segment ? "ABabcFGH" : "abc";
        auto const result = Complete(segment ? 206 : 200, QNetworkReply::NoError, std::move(options));
        TEST_EQUAL(result.m_success, !cancelled, (segment, signalReady, cancelled));
        TEST_EQUAL(result.m_errorCode, cancelled ? HttpClient::kCancelled : (segment ? 206 : 200), ());
        TEST(result.m_serverResponse.empty(), ());
      }
    }
  }
}

UNIT_TEST(HttpClientQt_SegmentLengthAtCompletion)
{
  for (bool const signalReady : {false, true})
  {
    for (auto const body : {"", "ab", "abcd"})
    {
      QTemporaryFile file;
      TEST(file.open(), ());
      auto const path = file.fileName().toStdString();
      file.close();
      auto const result = Complete(
          206, QNetworkReply::NoError,
          {.m_segment = HttpClient::ReceivedFileSegment{path, 2, 3, 8}, .m_signalReady = signalReady, .m_body = body});
      TEST(!result.m_success, (signalReady, body));
      TEST_EQUAL(result.m_errorCode, HttpClient::kInconsistentFileSize, ());
    }
  }
}

UNIT_TEST(HttpClientQt_StreamingAtCompletion)
{
  for (bool const accept : {false, true})
  {
    std::string bytes;
    auto const result = Complete(200, QNetworkReply::NoError,
                                 {.m_dataHandler =
                                      [&](void const * data, size_t size)
    {
      bytes.append(static_cast<char const *>(data), size);
      return accept;
    },
                                  .m_signalReady = false});
    TEST_EQUAL(bytes, "abc", ());
    TEST_EQUAL(result.m_success, accept, ());
    TEST(result.m_serverResponse.empty(), ());
  }
}

UNIT_TEST(HttpClientQt_SegmentHttpErrors)
{
  for (bool const signalReady : {false, true})
  {
    QTemporaryFile file;
    TEST(file.open(), ());
    TEST_EQUAL(file.write("ABCDEFGH"), 8, ());
    auto const path = file.fileName().toStdString();
    file.close();
    auto const result =
        Complete(404, QNetworkReply::ContentNotFoundError,
                 {.m_segment = HttpClient::ReceivedFileSegment{path, 2, 3, 8}, .m_signalReady = signalReady});
    TEST(!result.m_success, ());
    TEST_EQUAL(result.m_errorCode, 404, ());
    TEST(result.m_serverResponse.empty(), ());
    TEST(file.open(), ());
    TEST_EQUAL(file.readAll().toStdString(), "ABCDEFGH", ());
  }
}

enum class ResponseEnd
{
  Complete,
  Disconnect,
  Stall
};

HttpClient::Result Receive(int status, ResponseEnd end, std::string const & path = {},
                           HttpClient::DataHandler dataHandler = {})
{
  QTcpServer server;
  TEST(server.listen(QHostAddress::LocalHost), (server.errorString().toStdString()));
  QObject::connect(&server, &QTcpServer::newConnection, &server, [&]
  {
    auto * socket = server.nextPendingConnection();
    QObject::connect(socket, &QTcpSocket::readyRead, socket,
                     [socket, status, end, request = QByteArray(), sent = false]() mutable
    {
      request += socket->readAll();
      if (sent || !request.contains("\r\n\r\n"))
        return;
      sent = true;
      socket->write("HTTP/1.1 " + QByteArray::number(status) + " Test\r\nContent-Length: " +
                    (end == ResponseEnd::Complete ? "3" : "8") + "\r\nConnection: close\r\n\r\nabc");
      if (end != ResponseEnd::Stall)
        socket->disconnectFromHost();
      else
        socket->flush();
    });
  });

  HttpClient client("http://127.0.0.1:" + std::to_string(server.serverPort()) + "/completion");
  client.SetTimeout(1);
  if (!path.empty())
    client.SetReceivedFile(path);
  if (dataHandler)
    client.SetDataHandler(std::move(dataHandler));
  QEventLoop loop;
  std::optional<HttpClient::Result> result;
  client.RunHttpRequestAsync([&](HttpClient::Result value)
  {
    QMetaObject::invokeMethod(&loop, [&, value = std::move(value)]() mutable
    {
      result = std::move(value);
      loop.quit();
    }, Qt::QueuedConnection);
  });
  loop.exec();
  TEST(result, ());
  return std::move(*result);
}

UNIT_TEST(HttpClientQt_RealResponseCompletion)
{
  for (int const status : {200, 206, 404, 500})
  {
    for (auto const end : {ResponseEnd::Complete, ResponseEnd::Disconnect, ResponseEnd::Stall})
    {
      auto const result = Receive(status, end);
      TEST_EQUAL(result.m_success, end == ResponseEnd::Complete, (status));
      if (end == ResponseEnd::Complete)
      {
        TEST_EQUAL(result.m_errorCode, status, ());
        TEST_EQUAL(result.m_serverResponse, "abc", ());
      }
      else
        TEST_EQUAL(result.m_errorCode,
                   int(end == ResponseEnd::Stall ? QNetworkReply::TimeoutError : QNetworkReply::RemoteHostClosedError),
                   (status));
    }
  }
}

UNIT_TEST(HttpClientQt_RealInterruptedSinks)
{
  for (auto const end : {ResponseEnd::Disconnect, ResponseEnd::Stall})
  {
    QTemporaryFile file;
    TEST(file.open(), ());
    auto const path = file.fileName().toStdString();
    file.close();
    auto const result = Receive(200, end, path);
    TEST(!result.m_success, ());
    TEST(file.open(), ());
    TEST_EQUAL(file.readAll().toStdString(), "abc", ());

    std::string bytes;
    auto const streamed = Receive(200, end, {}, [&](void const * data, size_t size)
    {
      bytes.append(static_cast<char const *>(data), size);
      return true;
    });
    TEST(!streamed.m_success, ());
    TEST_EQUAL(bytes, "abc", ());
  }
}
}  // namespace http_client_qt_completion_test
