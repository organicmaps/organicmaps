#include "platform/http_client.hpp"

#include "base/string_utils.hpp"

#include "std/sstream.hpp"

namespace platform
{
HttpClient & HttpClient::SetDebugMode(bool debug_mode)
{
  m_debugMode = debug_mode;
  return *this;
}

HttpClient & HttpClient::SetUrlRequested(string const & url)
{
  m_urlRequested = url;
  return *this;
}

HttpClient & HttpClient::SetHttpMethod(string const & method)
{
  m_httpMethod = method;
  return *this;
}

HttpClient & HttpClient::SetBodyFile(string const & body_file, string const & content_type,
                                     string const & http_method /* = "POST" */,
                                     string const & content_encoding /* = "" */)
{
  m_inputFile = body_file;
  m_bodyData.clear();
  m_contentType = content_type;
  m_httpMethod = http_method;
  m_contentEncoding = content_encoding;
  return *this;
}

HttpClient & HttpClient::SetReceivedFile(string const & received_file)
{
  m_outputFile = received_file;
  return *this;
}

HttpClient & HttpClient::SetUserAgent(string const & user_agent)
{
  m_userAgent = user_agent;
  return *this;
}

HttpClient & HttpClient::SetUserAndPassword(string const & user, string const & password)
{
  m_basicAuthUser = user;
  m_basicAuthPassword = password;
  return *this;
}

HttpClient & HttpClient::SetCookies(string const & cookies)
{
  m_cookies = cookies;
  return *this;
}

HttpClient & HttpClient::SetHandleRedirects(bool handle_redirects)
{
  m_handleRedirects = handle_redirects;
  return *this;
}

string const & HttpClient::UrlRequested() const
{
  return m_urlRequested;
}

string const & HttpClient::UrlReceived() const
{
  return m_urlReceived;
}

bool HttpClient::WasRedirected() const
{
  return m_urlRequested != m_urlReceived;
}

int HttpClient::ErrorCode() const
{
  return m_errorCode;
}

string const & HttpClient::ServerResponse() const
{
  return m_serverResponse;
}

string const & HttpClient::HttpMethod() const
{
  return m_httpMethod;
}

string HttpClient::CombinedCookies() const
{
  if (m_serverCookies.empty())
    return m_cookies;

  if (m_cookies.empty())
    return m_serverCookies;

  return m_serverCookies + "; " + m_cookies;
}

string HttpClient::CookieByName(string name) const
{
  string const str = CombinedCookies();
  name += "=";
  auto const cookie = str.find(name);
  auto const eq = cookie + name.size();
  if (cookie != string::npos && str.size() > eq)
    return str.substr(eq, str.find(';', eq) - eq);

  return {};
}

// static
string HttpClient::NormalizeServerCookies(string && cookies)
{
  istringstream is(cookies);
  string str, result;

  // Split by ", ". Can have invalid tokens here, expires= can also contain a comma.
  while (getline(is, str, ','))
  {
    size_t const leading = str.find_first_not_of(" ");
    if (leading != string::npos)
      str.substr(leading).swap(str);

    // In the good case, we have '=' and it goes before any ' '.
    auto const eq = str.find('=');
    if (eq == string::npos)
      continue;  // It's not a cookie: no valid key value pair.

    auto const sp = str.find(' ');
    if (sp != string::npos && eq > sp)
      continue;  // It's not a cookie: comma in expires date.

    // Insert delimiter.
    if (!result.empty())
      result.append("; ");

    // Read cookie itself.
    result.append(str, 0, str.find(";"));
  }
  return result;
}

string DebugPrint(HttpClient const & request)
{
  ostringstream ostr;
  ostr << "HTTP " << request.ErrorCode() << " url [" << request.UrlRequested() << "]";
  if (request.WasRedirected())
    ostr << " was redirected to [" << request.UrlReceived() << "]";
  if (!request.ServerResponse().empty())
    ostr << " response: " << request.ServerResponse();
  return ostr.str();
}
}
