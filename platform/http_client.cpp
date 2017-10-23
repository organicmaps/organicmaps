#include "platform/http_client.hpp"

#include "coding/base64.hpp"

#include "base/string_utils.hpp"

#include "std/sstream.hpp"

namespace platform
{
HttpClient::HttpClient(string const & url) : m_urlRequested(url)
{
// Http client for linux supports only "deflate" encoding, but osrm server cannot
// correctly process "Accept-Encoding: deflate" header, so do not encode data on linux.
#if !defined(OMIM_OS_LINUX)
  m_headers.emplace("Accept-Encoding", "gzip, deflate");
#endif
}

bool HttpClient::RunHttpRequest(string & response, SuccessChecker checker /* = nullptr */)
{
  static auto const simpleChecker = [](HttpClient const & request)
  {
    return request.ErrorCode() == 200;
  };

  if (checker == nullptr)
    checker = simpleChecker;

  if (RunHttpRequest() && checker(*this))
  {
    response = ServerResponse();
    return true;
  }

  return false;
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
  m_headers.emplace("Content-Type", content_type);
  m_httpMethod = http_method;
  m_headers.emplace("Content-Encoding", content_encoding);
  return *this;
}

HttpClient & HttpClient::SetReceivedFile(string const & received_file)
{
  m_outputFile = received_file;
  return *this;
}

HttpClient & HttpClient::SetUserAndPassword(string const & user, string const & password)
{
  m_headers.emplace("Authorization", "Basic " + base64::Encode(user + ":" + password));
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

HttpClient & HttpClient::SetRawHeader(string const & key, string const & value)
{
  m_headers.emplace(key, value);
  return *this;
}

void HttpClient::SetTimeout(double timeoutSec)
{
  m_timeoutSec = timeoutSec;
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
  string serverCookies;
  auto const it = m_headers.find("Set-Cookie");
  if (it != m_headers.end())
    serverCookies = it->second;

  if (serverCookies.empty())
    return m_cookies;

  if (m_cookies.empty())
    return serverCookies;

  return serverCookies + "; " + m_cookies;
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

void HttpClient::LoadHeaders(bool loadHeaders)
{
  m_loadHeaders = loadHeaders;
}

unordered_map<string, string> const & HttpClient::GetHeaders() const
{
  return m_headers;
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
