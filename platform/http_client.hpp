/*******************************************************************************
The MIT License (MIT)

Copyright (c) 2015 Alexander Zolotarev <me@alex.bio> from Minsk, Belarus

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

#include "std/string.hpp"
#include "std/unordered_map.hpp"
#include "std/utility.hpp"

namespace platform
{
class HttpClient
{
public:
  static auto constexpr kNoError = -1;

  HttpClient() = default;
  explicit HttpClient(string const & url);

  // Synchronous (blocking) call, should be implemented for each platform
  // @returns true if connection was made and server returned something (200, 404, etc.).
  // @note Implementations should transparently support all needed HTTP redirects.
  // Implemented for each platform.
  bool RunHttpRequest();
  using SuccessChecker = std::function<bool(HttpClient const & request)>;
  // Returns true and copy of server response into [response] in case when RunHttpRequest() and
  // [checker] return true. When [checker] is equal to nullptr then default checker will be used.
  // Check by default: ErrorCode() == 200
  bool RunHttpRequest(string & response, SuccessChecker checker = nullptr);

  // Shared methods for all platforms, implemented at http_client.cpp
  HttpClient & SetDebugMode(bool debug_mode);
  HttpClient & SetUrlRequested(string const & url);
  HttpClient & SetHttpMethod(string const & method);
  // This method is mutually exclusive with set_body_data().
  HttpClient & SetBodyFile(string const & body_file, string const & content_type,
                           string const & http_method = "POST",
                           string const & content_encoding = "");
  // If set, stores server reply in file specified.
  HttpClient & SetReceivedFile(string const & received_file);
  // This method is mutually exclusive with set_body_file().
  template <typename StringT>
  HttpClient & SetBodyData(StringT && body_data, string const & content_type,
                           string const & http_method  = "POST",
                           string const & content_encoding = {})
  {
    m_bodyData = forward<StringT>(body_data);
    m_inputFile.clear();
    m_headers.emplace("Content-Type", content_type);
    m_httpMethod = http_method;
    if (!content_encoding.empty())
      m_headers.emplace("Content-Encoding", content_encoding);
    return *this;
  }
  // HTTP Basic Auth.
  HttpClient & SetUserAndPassword(string const & user, string const & password);
  // Set HTTP Cookie header.
  HttpClient & SetCookies(string const & cookies);
  // When set to true (default), clients never get 3XX codes from servers, redirects are handled automatically.
  // TODO: "false" is now supported on Android only.
  HttpClient & SetHandleRedirects(bool handle_redirects);
  HttpClient & SetRawHeader(string const & key, string const & value);
  void SetTimeout(double timeoutSec);

  string const & UrlRequested() const;
  // @returns empty string in the case of error
  string const & UrlReceived() const;
  bool WasRedirected() const;
  // Mix of HTTP errors (in case of successful connection) and system-dependent error codes,
  // in the simplest success case use 'if (200 == client.error_code())' // 200 means OK in HTTP
  int ErrorCode() const;
  string const & ServerResponse() const;
  string const & HttpMethod() const;
  // Pass this getter's value to the set_cookies() method for easier cookies support in the next request.
  string CombinedCookies() const;
  // Returns cookie value or empty string if it's not present.
  string CookieByName(string name) const;
  void LoadHeaders(bool loadHeaders);
  unordered_map<string, string> const & GetHeaders() const;

private:
  // Internal helper to convert cookies like this:
  // "first=value1; expires=Mon, 26-Dec-2016 12:12:32 GMT; path=/, second=value2; path=/, third=value3; "
  // into this:
  // "first=value1; second=value2; third=value3"
  static string NormalizeServerCookies(string && cookies);

  string m_urlRequested;
  // Contains final content's url taking redirects (if any) into an account.
  string m_urlReceived;
  int m_errorCode = kNoError;
  string m_inputFile;
  // Used instead of server_reply_ if set.
  string m_outputFile;
  // Data we received from the server if output_file_ wasn't initialized.
  string m_serverResponse;
  string m_bodyData;
  string m_httpMethod = "GET";
  // Cookies set by the client before request is run.
  string m_cookies;
  unordered_map<string, string> m_headers;
  bool m_handleRedirects = true;
  bool m_loadHeaders = false;
  // Use 30 seconds timeout by default.
  double m_timeoutSec = 30.0;

  DISALLOW_COPY_AND_MOVE(HttpClient);
};

string DebugPrint(HttpClient const & request);
}  // namespace platform
