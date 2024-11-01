#pragma once

#include "base/macros.hpp"

#include <functional>
#include <string>
#include <unordered_map>
#include <utility>

namespace om::network::http
{
class Client
{
public:
  static auto constexpr kNoError = -1;

  struct Header
  {
    std::string m_name;
    std::string m_value;
  };

  using Headers = std::unordered_map<std::string, std::string>;
  using SuccessChecker = std::function<bool(Client const & request)>;

  explicit Client(std::string const & url);

  // Synchronous (blocking) call, should be implemented for each platform
  // @returns true if connection was made and server returned something (200, 404, etc.).
  // @note Implementations should transparently support all needed HTTP redirects.
  // Implemented for each platform.
  bool RunHttpRequest();

  // Returns true and copy of server response into [response] in case when RunHttpRequest() and
  // [checker] return true. When [checker] is equal to nullptr then default checker will be used.
  // Check by default: ErrorCode() == 200
  bool RunHttpRequest(std::string & response, SuccessChecker checker = nullptr);

  Client & SetUrlRequested(std::string const & url);
  Client & SetHttpMethod(std::string const & method);
  // This method is mutually exclusive with set_body_data().
  Client & SetBodyFile(std::string const & body_file, std::string const & content_type,
                       std::string const & http_method = "POST", std::string const & content_encoding = "");
  // If set, stores server reply in file specified.
  Client & SetReceivedFile(std::string const & received_file);
  // This method is mutually exclusive with set_body_file().
  template <typename StringT>
  Client & SetBodyData(StringT && body_data, std::string const & content_type, std::string const & http_method = "POST",
                       std::string const & content_encoding = {})
  {
    m_bodyData = std::forward<StringT>(body_data);
    m_inputFile.clear();
    m_headers.emplace("Content-Type", content_type);
    m_httpMethod = http_method;
    if (!content_encoding.empty())
      m_headers.emplace("Content-Encoding", content_encoding);
    return *this;
  }
  // HTTP Basic Auth.
  Client & SetUserAndPassword(std::string const & user, std::string const & password);
  // Set HTTP Cookie header.
  Client & SetCookies(std::string const & cookies);
  // When set to false (default), clients never get 3XX codes from servers, redirects are handled automatically.
  Client & SetFollowRedirects(bool follow_redirects);
  Client & SetRawHeader(std::string const & key, std::string const & value);
  Client & SetRawHeaders(Headers const & headers);
  void SetTimeout(double timeoutSec);

  std::string const & UrlRequested() const;
  // @returns empty string in the case of error
  std::string const & UrlReceived() const;
  bool WasRedirected() const;
  // Mix of HTTP errors (in case of successful connection) and system-dependent error codes,
  // in the simplest success case use 'if (200 == client.error_code())' // 200 means OK in HTTP
  int ErrorCode() const;
  std::string const & ServerResponse() const;
  std::string const & HttpMethod() const;
  // Pass this getter's value to the set_cookies() method for easier cookies support in the next request.
  std::string CombinedCookies() const;
  // Returns cookie value or empty string if it's not present.
  std::string CookieByName(std::string name) const;
  void LoadHeaders(bool loadHeaders);
  Headers const & GetHeaders() const;

private:
  // Internal helper to convert cookies like this:
  // "first=value1; expires=Mon, 26-Dec-2016 12:12:32 GMT; path=/, second=value2; path=/, third=value3; "
  // into this:
  // "first=value1; second=value2; third=value3"
  static std::string NormalizeServerCookies(std::string && cookies);

  std::string m_urlRequested;
  // Contains final content's url taking redirects (if any) into an account.
  std::string m_urlReceived;
  int m_errorCode = kNoError;
  std::string m_inputFile;
  // Used instead of server_reply_ if set.
  std::string m_outputFile;
  // Data we received from the server if output_file_ wasn't initialized.
  std::string m_serverResponse;
  std::string m_bodyData;
  std::string m_httpMethod = "GET";
  // Cookies set by the client before request is run.
  std::string m_cookies;
  Headers m_headers;
  bool m_followRedirects =
      false;  // If true then in case of HTTP response 3XX make another request to follow redirected URL
  bool m_loadHeaders = false;
  // Use 30 seconds timeout by default.
  double m_timeoutSec = 30.0;

  DISALLOW_COPY_AND_MOVE(Client);
};

std::string DebugPrint(Client const & request);
}  // namespace om::network::http
