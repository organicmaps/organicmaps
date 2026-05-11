#include "testing/testing.hpp"

#include "platform/http_client_qt.hpp"

namespace http_client_qt_retry_test
{
using platform::HttpClientReply;

UNIT_TEST(HttpClientQt_ShouldRetry_ContentReSendError)
{
  TEST(HttpClientReply::ShouldRetry(QNetworkReply::ContentReSendError, "", 0,
                                    /*anyBytesReceived*/ false, /*writeError*/ false,
                                    /*retriesRemaining*/ 1),
       ());
}

UNIT_TEST(HttpClientQt_ShouldRetry_ProtocolFailureWithGoAwayString)
{
  TEST(HttpClientReply::ShouldRetry(QNetworkReply::ProtocolFailure,
                                    "stream 9 finished with error: Server stopped accepting new streams "
                                    "before this stream was established",
                                    0, false, false, 1),
       ());
  TEST(HttpClientReply::ShouldRetry(QNetworkReply::ProtocolFailure, "received REFUSED_STREAM", 0, false, false, 1), ());
  TEST(HttpClientReply::ShouldRetry(QNetworkReply::ProtocolFailure, "GOAWAY received", 0, false, false, 1), ());
  // Case-insensitive match.
  TEST(HttpClientReply::ShouldRetry(QNetworkReply::ProtocolFailure, "refused_stream", 0, false, false, 1), ());
}

UNIT_TEST(HttpClientQt_ShouldRetry_ProtocolFailureWithoutGoAwayString)
{
  // Generic protocol failure without GOAWAY/REFUSED_STREAM markers — don't retry blindly.
  TEST(!HttpClientReply::ShouldRetry(QNetworkReply::ProtocolFailure, "malformed HTTP response", 0, false, false, 1),
       ());
  TEST(!HttpClientReply::ShouldRetry(QNetworkReply::ProtocolFailure, "", 0, false, false, 1), ());
}

UNIT_TEST(HttpClientQt_ShouldRetry_NoErrorMeansSuccess)
{
  TEST(!HttpClientReply::ShouldRetry(QNetworkReply::NoError, "", 200, false, false, 1), ());
}

UNIT_TEST(HttpClientQt_ShouldRetry_AnyHttpStatusMeansServerProcessedRequest)
{
  // Server returned an HTTP response of any kind — request was processed, don't retry transparently.
  TEST(!HttpClientReply::ShouldRetry(QNetworkReply::ContentReSendError, "", 503, false, false, 1), ());
  TEST(!HttpClientReply::ShouldRetry(QNetworkReply::ProtocolFailure, "GOAWAY", 200, false, false, 1), ());
  TEST(!HttpClientReply::ShouldRetry(QNetworkReply::ProtocolFailure, "GOAWAY", 404, false, false, 1), ());
}

UNIT_TEST(HttpClientQt_ShouldRetry_NotAfterBytesReceived)
{
  TEST(!HttpClientReply::ShouldRetry(QNetworkReply::ContentReSendError, "", 0,
                                     /*anyBytesReceived*/ true, false, 1),
       ());
  TEST(!HttpClientReply::ShouldRetry(QNetworkReply::ProtocolFailure, "GOAWAY", 0,
                                     /*anyBytesReceived*/ true, false, 1),
       ());
}

UNIT_TEST(HttpClientQt_ShouldRetry_NotOnLocalWriteError)
{
  TEST(!HttpClientReply::ShouldRetry(QNetworkReply::ContentReSendError, "", 0, false,
                                     /*writeError*/ true, 1),
       ());
}

UNIT_TEST(HttpClientQt_ShouldRetry_NotWhenBudgetExhausted)
{
  TEST(!HttpClientReply::ShouldRetry(QNetworkReply::ContentReSendError, "", 0, false, false,
                                     /*retriesRemaining*/ 0),
       ());
  TEST(!HttpClientReply::ShouldRetry(QNetworkReply::ContentReSendError, "", 0, false, false, -1), ());
}

UNIT_TEST(HttpClientQt_ShouldRetry_UnrelatedErrorsDoNotRetry)
{
  // Errors that don't match the GOAWAY/REFUSED_STREAM pattern must not retry —
  // they cover cases where the server may have processed the request (timeout
  // during response, connection reset mid-stream, etc.).
  TEST(!HttpClientReply::ShouldRetry(QNetworkReply::TimeoutError, "", 0, false, false, 1), ());
  TEST(!HttpClientReply::ShouldRetry(QNetworkReply::HostNotFoundError, "", 0, false, false, 1), ());
  TEST(!HttpClientReply::ShouldRetry(QNetworkReply::ConnectionRefusedError, "", 0, false, false, 1), ());
  TEST(!HttpClientReply::ShouldRetry(QNetworkReply::OperationCanceledError, "", 0, false, false, 1), ());
  TEST(!HttpClientReply::ShouldRetry(QNetworkReply::RemoteHostClosedError, "", 0, false, false, 1), ());
  TEST(!HttpClientReply::ShouldRetry(QNetworkReply::SslHandshakeFailedError, "", 0, false, false, 1), ());
  TEST(!HttpClientReply::ShouldRetry(QNetworkReply::TemporaryNetworkFailureError, "", 0, false, false, 1), ());
}
}  // namespace http_client_qt_retry_test
