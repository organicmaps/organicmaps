/*******************************************************************************
The MIT License (MIT)

Copyright (c) 2015 Alexander Borsuk <me@alex.bio> from Minsk, Belarus

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*******************************************************************************/
#pragma once

#include "base/macros.hpp"

#include <functional>
#include <string>
#include <unordered_map>
#include <utility>

namespace platform
{
class HttpClient
{
public:
  static auto constexpr kNoError = -1;

  struct Header
  {
    std::string m_name;
    std::string m_value;
  };

  using Headers = std::unordered_map<std::string, std::string>;

  HttpClient() = default;
  explicit HttpClient(std::string const & url);

  // Synchronous (blocking) call, should be implemented for each platform
  // @returns true if connection was made and server returned something (200, 404, etc.).
  // @note Implementations should transparently support all needed HTTP redirects.
  // Implemented for each platform.
  bool RunHttpRequest();
  using SuccessChecker = std::function<bool(HttpClient const & request)>;
  // Returns true and copy of server response into [response] in case when RunHttpRequest() and
  // [checker] return true. When [checker] is equal to nullptr then default checker will be used.
  // Check by default: ErrorCode() == 200
  bool RunHttpRequest(std::string & response, SuccessChecker checker = nullptr);

  HttpClient & SetUrlRequested(std::string const & url);
  HttpClient & SetHttpMethod(std::string const & method);
  // This method is mutually exclusive with set_body_data().
  HttpClient & SetBodyFile(std::string const & body_file, std::string const & content_type,
                           std::string const & http_method = "POST", std::string const & content_encoding = "");
  // If set, stores server reply in file specified.
  HttpClient & SetReceivedFile(std::string const & received_file);
  // This method is mutually exclusive with set_body_file().
  template <typename StringT>
  HttpClient & SetBodyData(StringT && body_data, std::string const & content_type,
                           std::string const & http_method = "POST", std::string const & content_encoding = {})
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
  HttpClient & SetUserAndPassword(std::string const & user, std::string const & password);
  // Set HTTP Cookie header.
  HttpClient & SetCookies(std::string const & cookies);
  // When set to false (default), clients never get 3XX codes from servers, redirects are handled automatically.
  HttpClient & SetFollowRedirects(bool follow_redirects);
  HttpClient & SetRawHeader(std::string const & key, std::string const & value);
  HttpClient & SetRawHeaders(Headers const & headers);
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

  DISALLOW_COPY_AND_MOVE(HttpClient);
};

std::string DebugPrint(HttpClient const & request);
}  // namespace platform
