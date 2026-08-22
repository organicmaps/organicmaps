#include "testing/testing.hpp"

#include "platform/http_client_qt.hpp"

namespace http_client_qt_response_test
{
using platform::HttpClientReply;

UNIT_TEST(HttpClientQt_IsCompleteResponse_NoError)
{
  TEST(HttpClientReply::IsCompleteResponse(QNetworkReply::NoError, 200), ());
  TEST(HttpClientReply::IsCompleteResponse(QNetworkReply::NoError, 204), ());
}

UNIT_TEST(HttpClientQt_IsCompleteResponse_NoStatusLine)
{
  // Nothing was received, so there is no response to be complete.
  TEST(!HttpClientReply::IsCompleteResponse(QNetworkReply::NoError, 0), ());
  TEST(!HttpClientReply::IsCompleteResponse(QNetworkReply::HostNotFoundError, 0), ());
  TEST(!HttpClientReply::IsCompleteResponse(QNetworkReply::ConnectionRefusedError, 0), ());
  TEST(!HttpClientReply::IsCompleteResponse(QNetworkReply::TimeoutError, 0), ());
}

UNIT_TEST(HttpClientQt_IsCompleteResponse_HttpStatusErrorsAreComplete)
{
  // These pairs match Qt's HTTP-status-to-NetworkError mapping.
  TEST(HttpClientReply::IsCompleteResponse(QNetworkReply::ProtocolInvalidOperationError, 400), ());
  TEST(HttpClientReply::IsCompleteResponse(QNetworkReply::ContentAccessDenied, 403), ());
  TEST(HttpClientReply::IsCompleteResponse(QNetworkReply::ContentNotFoundError, 404), ());
  TEST(HttpClientReply::IsCompleteResponse(QNetworkReply::AuthenticationRequiredError, 401), ());
  TEST(HttpClientReply::IsCompleteResponse(QNetworkReply::ContentOperationNotPermittedError, 405), ());
  TEST(HttpClientReply::IsCompleteResponse(QNetworkReply::ProxyAuthenticationRequiredError, 407), ());
  TEST(HttpClientReply::IsCompleteResponse(QNetworkReply::ContentConflictError, 409), ());
  TEST(HttpClientReply::IsCompleteResponse(QNetworkReply::ContentGoneError, 410), ());
  TEST(HttpClientReply::IsCompleteResponse(QNetworkReply::ProtocolInvalidOperationError, 418), ());
  TEST(HttpClientReply::IsCompleteResponse(QNetworkReply::UnknownContentError, 429), ());
  TEST(HttpClientReply::IsCompleteResponse(QNetworkReply::InternalServerError, 500), ());
  TEST(HttpClientReply::IsCompleteResponse(QNetworkReply::OperationNotImplementedError, 501), ());
  TEST(HttpClientReply::IsCompleteResponse(QNetworkReply::UnknownServerError, 502), ());
  TEST(HttpClientReply::IsCompleteResponse(QNetworkReply::ServiceUnavailableError, 503), ());
  TEST(HttpClientReply::IsCompleteResponse(QNetworkReply::UnknownServerError, 599), ());
}

UNIT_TEST(HttpClientQt_IsCompleteResponse_TransportFailureAfterStatusLine)
{
  // Transport and content-processing errors can arrive after an ordinary status line.
  TEST(!HttpClientReply::IsCompleteResponse(QNetworkReply::TimeoutError, 200), ());
  TEST(!HttpClientReply::IsCompleteResponse(QNetworkReply::OperationCanceledError, 200), ());
  TEST(!HttpClientReply::IsCompleteResponse(QNetworkReply::RemoteHostClosedError, 200), ());
  TEST(!HttpClientReply::IsCompleteResponse(QNetworkReply::TemporaryNetworkFailureError, 200), ());
  TEST(!HttpClientReply::IsCompleteResponse(QNetworkReply::NetworkSessionFailedError, 206), ());
  // HTTP/2 stream torn down mid-body after the headers arrived.
  TEST(!HttpClientReply::IsCompleteResponse(QNetworkReply::ProtocolFailure, 200), ());
  TEST(!HttpClientReply::IsCompleteResponse(QNetworkReply::SslHandshakeFailedError, 200), ());
  TEST(!HttpClientReply::IsCompleteResponse(QNetworkReply::UnknownContentError, 200), ());
  TEST(!HttpClientReply::IsCompleteResponse(QNetworkReply::ContentReSendError, 200), ());
  // A status-mapping error paired with a different status indicates another failure.
  TEST(!HttpClientReply::IsCompleteResponse(QNetworkReply::UnknownContentError, 404), ());
  TEST(!HttpClientReply::IsCompleteResponse(QNetworkReply::ContentNotFoundError, 200), ());
}
}  // namespace http_client_qt_response_test
