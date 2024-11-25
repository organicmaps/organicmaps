#include "client.hpp"

#include "coding/base64.hpp"

namespace om::network::http
{
Client::Client(std::string const & url) : m_urlRequested(url) {}

bool Client::RunHttpRequest(std::string & response, SuccessChecker checker /* = nullptr */)
{
  static auto const simpleChecker = [](Client const & request) { return request.ErrorCode() == 200; };

  if (checker == nullptr)
    checker = simpleChecker;

  if (RunHttpRequest() && checker(*this))
  {
    response = ServerResponse();
    return true;
  }

  return false;
}

Client & Client::SetUrlRequested(std::string const & url)
{
  m_urlRequested = url;
  return *this;
}

Client & Client::SetHttpMethod(std::string const & method)
{
  m_httpMethod = method;
  return *this;
}

Client & Client::SetBodyFile(std::string const & body_file, std::string const & content_type,
                             std::string const & http_method /* = "POST" */,
                             std::string const & content_encoding /* = "" */)
{
  m_inputFile = body_file;
  m_bodyData.clear();
  m_headers.emplace("Content-Type", content_type);
  m_httpMethod = http_method;
  m_headers.emplace("Content-Encoding", content_encoding);
  return *this;
}

Client & Client::SetReceivedFile(std::string const & received_file)
{
  m_outputFile = received_file;
  return *this;
}

Client & Client::SetUserAndPassword(std::string const & user, std::string const & password)
{
  m_headers.emplace("Authorization", "Basic " + base64::Encode(user + ":" + password));
  return *this;
}

Client & Client::SetCookies(std::string const & cookies)
{
  m_cookies = cookies;
  return *this;
}

Client & Client::SetFollowRedirects(bool followRedirects)
{
  m_followRedirects = followRedirects;
  return *this;
}

Client & Client::SetRawHeader(std::string const & key, std::string const & value)
{
  m_headers.emplace(key, value);
  return *this;
}

Client & Client::SetRawHeaders(Headers const & headers)
{
  m_headers.insert(headers.begin(), headers.end());
  return *this;
}

void Client::SetTimeout(double timeoutSec) { m_timeoutSec = timeoutSec; }

std::string const & Client::UrlRequested() const { return m_urlRequested; }

std::string const & Client::UrlReceived() const { return m_urlReceived; }

bool Client::WasRedirected() const { return m_urlRequested != m_urlReceived; }

int Client::ErrorCode() const { return m_errorCode; }

std::string const & Client::ServerResponse() const { return m_serverResponse; }

std::string const & Client::HttpMethod() const { return m_httpMethod; }

std::string Client::CombinedCookies() const
{
  std::string serverCookies;
  auto const it = m_headers.find("Set-Cookie");
  if (it != m_headers.end())
    serverCookies = it->second;

  if (serverCookies.empty())
    return m_cookies;

  if (m_cookies.empty())
    return serverCookies;

  return serverCookies + "; " + m_cookies;
}

std::string Client::CookieByName(std::string name) const
{
  std::string const str = CombinedCookies();
  name += "=";
  auto const cookie = str.find(name);
  auto const eq = cookie + name.size();
  if (cookie != std::string::npos && str.size() > eq)
    return str.substr(eq, str.find(';', eq) - eq);

  return {};
}

void Client::LoadHeaders(bool loadHeaders) { m_loadHeaders = loadHeaders; }

Client::Headers const & Client::GetHeaders() const { return m_headers; }

// static
std::string Client::NormalizeServerCookies(std::string && cookies)
{
  std::istringstream is(cookies);
  std::string str, result;

  // Split by ", ". Can have invalid tokens here, expires= can also contain a comma.
  while (getline(is, str, ','))
  {
    size_t const leading = str.find_first_not_of(' ');
    if (leading != std::string::npos)
      str.substr(leading).swap(str);

    // In the good case, we have '=' and it goes before any ' '.
    auto const eq = str.find('=');
    if (eq == std::string::npos)
      continue;  // It's not a cookie: no valid key value pair.

    auto const sp = str.find(' ');
    if (sp != std::string::npos && eq > sp)
      continue;  // It's not a cookie: comma in expires date.

    // Insert delimiter.
    if (!result.empty())
      result.append("; ");

    // Read cookie itself.
    result.append(str, 0, str.find(';'));
  }
  return result;
}

std::string DebugPrint(Client const & request)
{
  std::ostringstream ostr;
  ostr << "HTTP " << request.ErrorCode() << " url [" << request.UrlRequested() << "]";
  if (request.WasRedirected())
    ostr << " was redirected to [" << request.UrlReceived() << "]";
  if (!request.ServerResponse().empty())
    ostr << " response: " << request.ServerResponse();
  return ostr.str();
}
}  // namespace om::network::http
