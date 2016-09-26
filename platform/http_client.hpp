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

#include "std/sstream.hpp"
#include "std/string.hpp"

namespace platform
{
class HttpClient
{
 public:
  enum { kNotInitialized = -1 };

  HttpClient() = default;
  HttpClient(string const & url) : m_urlRequested(url) {}
  HttpClient & set_debug_mode(bool debug_mode)
  {
    m_debugMode = debug_mode;
    return *this;
  }
  HttpClient & set_url_requested(string const & url)
  {
    m_urlRequested = url;
    return *this;
  }
  HttpClient & set_http_method(string const & method)
  {
    m_httpMethod = method;
    return *this;
  }
  // This method is mutually exclusive with set_body_data().
  HttpClient & set_body_file(string const & body_file,
                             string const & content_type,
                             string const & http_method = "POST",
                             string const & content_encoding = "")
  {
    m_bodyFile = body_file;
    m_bodyData.clear();
    m_contentType = content_type;
    m_httpMethod = http_method;
    m_contentEncoding = content_encoding;
    return *this;
  }
  // If set, stores server reply in file specified.
  HttpClient & set_received_file(string const & received_file)
  {
    m_receivedFile = received_file;
    return *this;
  }  
  HttpClient & set_user_agent(string const & user_agent)
  {
    m_userAgent = user_agent;
    return *this;
  }
  // This method is mutually exclusive with set_body_file().
  HttpClient & set_body_data(string const & body_data,
                             string const & content_type,
                             string const  & http_method = "POST",
                             string const & content_encoding = "")
  {
    m_bodyData = body_data;
    m_bodyFile.clear();
    m_contentType = content_type;
    m_httpMethod = http_method;
    m_contentEncoding = content_encoding;
    return *this;
  }
  // Move version to avoid string copying.
  // This method is mutually exclusive with set_body_file().
  HttpClient & set_body_data(string && body_data,
                             string const & content_type,
                             string const & http_method = "POST",
                             string const & content_encoding = "")
  {
    m_bodyData = move(body_data);
    m_bodyFile.clear();
    m_contentType = content_type;
    m_httpMethod = http_method;
    m_contentEncoding = content_encoding;
    return *this;
  }
  // HTTP Basic Auth.
  HttpClient & set_user_and_password(string const & user, string const & password)
  {
    m_basicAuthUser = user;
    m_basicAuthPassword = password;
    return *this;
  }
  // Set HTTP Cookie header.
  HttpClient & set_cookies(string const & cookies)
  {
    m_cookies = cookies;
    return *this;
  }
  // When set to true (default), clients never get 3XX codes from servers, redirects are handled automatically.
  // TODO: "false" is now supported on Android only.
  HttpClient & set_handle_redirects(bool handle_redirects)
  {
    m_handleRedirects = handle_redirects;
    return *this;
  }

  // Synchronous (blocking) call, should be implemented for each platform
  // @returns true if connection was made and server returned something (200, 404, etc.).
  // @note Implementations should transparently support all needed HTTP redirects
  bool RunHttpRequest();

  string const & url_requested() const { return m_urlRequested; }
  // @returns empty string in the case of error
  string const & url_received() const { return m_urlReceived; }
  bool was_redirected() const { return m_urlRequested != m_urlReceived; }
  // Mix of HTTP errors (in case of successful connection) and system-dependent error codes,
  // in the simplest success case use 'if (200 == client.error_code())' // 200 means OK in HTTP
  int error_code() const { return m_errorCode; }
  string const & server_response() const { return m_serverResponse; }
  string const & http_method() const { return m_httpMethod; }
  // Pass this getter's value to the set_cookies() method for easier cookies support in the next request.
  string combined_cookies() const
  {
    if (m_serverCookies.empty())
    {
      return m_cookies;
    }
    if (m_cookies.empty())
    {
      return m_serverCookies;
    }
    return m_serverCookies + "; " + m_cookies;
  }
  // Returns cookie value or empty string if it's not present.
  string cookie_by_name(string name) const
  {
    string const str = combined_cookies();
    name += "=";
    auto const cookie = str.find(name);
    auto const eq = cookie + name.size();
    if (cookie != string::npos && str.size() > eq)
    {
      return str.substr(eq, str.find(';', eq) - eq);
    }
    return string();
  }

private:
  // Internal helper to convert cookies like this:
  // "first=value1; expires=Mon, 26-Dec-2016 12:12:32 GMT; path=/, second=value2; path=/, third=value3; "
  // into this:
  // "first=value1; second=value2; third=value3"
  static string normalize_server_cookies(string && cookies)
  {
    istringstream is(cookies);
    string str, result;
    // Split by ", ". Can have invalid tokens here, expires= can also contain a comma.
    while (getline(is, str, ','))
    {
      size_t const leading = str.find_first_not_of(" ");
      if (leading != string::npos)
      {
        str.substr(leading).swap(str);
      }
      // In the good case, we have '=' and it goes before any ' '.
      auto const eq = str.find('=');
      if (eq == string::npos)
      {
        continue;  // It's not a cookie: no valid key value pair.
      }
      auto const sp = str.find(' ');
      if (sp != string::npos && eq > sp)
      {
        continue;  // It's not a cookie: comma in expires date.
      }
      // Insert delimiter.
      if (!result.empty())
      {
        result.append("; ");
      }
      // Read cookie itself.
      result.append(str, 0, str.find(";"));
    }
    return result;
  }

 string m_urlRequested;
 // Contains final content's url taking redirects (if any) into an account.
 string m_urlReceived;
 int m_errorCode = kNotInitialized;
 string m_bodyFile;
 // Used instead of server_reply_ if set.
 string m_receivedFile;
 // Data we received from the server if output_file_ wasn't initialized.
 string m_serverResponse;
 string m_contentType;
 string m_contentTypeReceived;
 string m_contentEncoding;
 string m_contentEncodingReceived;
 string m_userAgent;
 string m_bodyData;
 string m_httpMethod = "GET";
 string m_basicAuthUser;
 string m_basicAuthPassword;
 // All Set-Cookie values from server response combined in a Cookie format:
 // cookie1=value1; cookie2=value2
 // TODO(AlexZ): Support encoding and expiration/path/domains etc.
 string m_serverCookies;
 // Cookies set by the client before request is run.
 string m_cookies;
 bool m_debugMode = false;
 bool m_handleRedirects = true;

 DISALLOW_COPY_AND_MOVE(HttpClient);
};
}  // namespace platform
