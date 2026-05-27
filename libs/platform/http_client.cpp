#include "platform/http_client.hpp"

#include "coding/base64.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <cctype>
#include <condition_variable>
#include <ranges>
#include <sstream>

namespace platform
{
using std::string;

HttpClient::HttpClient(string const & url) : m_urlRequested(url) {}

void HttpClient::RequestHandle::Cancel()
{
  if (auto impl = m_impl)
  {
    impl->m_cancelled.store(true, std::memory_order_release);
    std::lock_guard lock(impl->m_mu);
    if (impl->m_platformCancel)
    {
      impl->m_platformCancel();
      impl->m_platformCancel = nullptr;
    }
  }
}

bool HttpClient::RequestHandle::IsCancelled() const
{
  if (auto impl = m_impl)
    return impl->m_cancelled.load(std::memory_order_acquire);
  return false;
}

HttpClient::CancelChecker HttpClient::RequestHandle::MakeCancelChecker() const
{
  return [implWeak = std::weak_ptr<Impl>(m_impl)]() -> bool
  {
    auto impl = implWeak.lock();
    return impl && impl->m_cancelled.load(std::memory_order_acquire);
  };
}

bool HttpClient::RunHttpRequest()
{
  std::mutex mu;
  std::condition_variable cv;
  bool done = false;
  Result result;

  RunHttpRequestAsync([&](Result r)
  {
    std::lock_guard lock(mu);
    result = std::move(r);
    done = true;
    cv.notify_one();
  });

  std::unique_lock lock(mu);
  cv.wait(lock, [&] { return done; });

  m_errorCode = result.m_errorCode;
  m_urlReceived = std::move(result.m_urlReceived);
  m_serverResponse = std::move(result.m_serverResponse);
  m_headers = std::move(result.m_headers);
  return result.m_success;
}

bool HttpClient::RunHttpRequest(string & response, SuccessChecker checker /* = nullptr */)
{
  static auto const simpleChecker = [](HttpClient const & request) { return request.ErrorCode() == 200; };

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
  if (!content_encoding.empty())
    m_headers.emplace("Content-Encoding", content_encoding);
  return *this;
}

HttpClient & HttpClient::SetReceivedFile(string const & received_file)
{
  m_outputFile = received_file;
  return *this;
}

HttpClient & HttpClient::SetReceivedFileSegment(ReceivedFileSegment segment)
{
  CHECK(!m_dataHandler, ("SetReceivedFileSegment is mutually exclusive with SetDataHandler"));
  m_receivedFileSegment = std::move(segment);
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

HttpClient & HttpClient::SetFollowRedirects(bool followRedirects)
{
  m_followRedirects = followRedirects;
  return *this;
}

HttpClient & HttpClient::SetRawHeader(string const & key, string const & value)
{
  m_headers.emplace(key, value);
  return *this;
}

HttpClient & HttpClient::SetRawHeaders(Headers const & headers)
{
  m_headers.insert(headers.begin(), headers.end());
  return *this;
}

HttpClient & HttpClient::SetTimeout(double timeoutSec)
{
  m_timeoutSec = timeoutSec;
  return *this;
}

HttpClient & HttpClient::SetProgressHandler(ProgressHandler handler)
{
  m_progressHandler = std::move(handler);
  return *this;
}

HttpClient & HttpClient::SetDataHandler(DataHandler handler)
{
  CHECK(!m_receivedFileSegment, ("SetDataHandler is mutually exclusive with SetReceivedFileSegment"));
  m_dataHandler = std::move(handler);
  return *this;
}

HttpClient & HttpClient::SetRange(int64_t begin, int64_t end)
{
  string rangeValue = "bytes=" + std::to_string(begin);
  if (end >= 0)
    rangeValue += "-" + std::to_string(end);
  else
    rangeValue += "-";
  m_headers["Range"] = std::move(rangeValue);
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
  string serverCookies;
  auto const it = m_headers.find("set-cookie");
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

HttpClient::Headers const & HttpClient::GetHeaders() const
{
  return m_headers;
}

// static
bool HttpClient::ParseContentRange(std::string_view header, int64_t & start, int64_t & end, int64_t & total)
{
  strings::Trim(header);
  // Strip the "bytes " prefix (case-insensitive per RFC 7233).
  if (header.size() >= 6 &&
      std::ranges::equal(header | std::views::take(6), std::string_view{"bytes "},
                         [](char a, char b) { return std::tolower(static_cast<unsigned char>(a)) == b; }))
  {
    header.remove_prefix(6);
    strings::Trim(header);
  }
  auto const dash = header.find('-');
  auto const slash = header.find('/');
  if (dash == std::string_view::npos || slash == std::string_view::npos || dash >= slash)
    return false;
  if (!strings::to_int(header.substr(0, dash), start))
    return false;
  if (!strings::to_int(header.substr(dash + 1, slash - dash - 1), end))
    return false;
  auto const totalStr = header.substr(slash + 1);
  if (totalStr == "*")
    total = -1;
  else if (!strings::to_int(totalStr, total))
    return false;
  return true;
}

// static
HttpClient::ReceivedFileSegmentValidation HttpClient::ValidateReceivedFileSegmentResponse(
    int httpCode, std::string_view contentRange, ReceivedFileSegment const & segment)
{
  if (httpCode != 206)
  {
    // 2xx-without-206 means the server ignored the Range header. Preserve 4xx/5xx so
    // higher layers can still distinguish real HTTP failures such as 404.
    if (httpCode >= 200 && httpCode < 300)
    {
      LOG(LWARNING, ("Segment response protocol violation: expected HTTP 206, got", httpCode));
      return {false, kInconsistentFileSize};
    }
    return {false, httpCode};
  }

  int64_t start = 0;
  int64_t end = 0;
  int64_t total = -1;
  if (contentRange.empty() || !ParseContentRange(contentRange, start, end, total))
  {
    LOG(LWARNING, ("Invalid or missing Content-Range for segment response:", std::string(contentRange)));
    return {false, kInconsistentFileSize};
  }

  int64_t const expectedEnd = segment.m_offset + segment.m_expectedBytes - 1;
  if (start != segment.m_offset || end != expectedEnd)
  {
    LOG(LWARNING, ("Content-Range mismatch: got", start, "-", end, "expected", segment.m_offset, "-", expectedEnd));
    return {false, kInconsistentFileSize};
  }

  // If the caller supplied an expected total file size, the server must echo it back.
  // A "*" would let a mirror serve a chunk from another file version at the same offset.
  if (segment.m_expectedTotalBytes >= 0 && total < 0)
  {
    LOG(LWARNING, ("Server sent Content-Range with unknown total, expected", segment.m_expectedTotalBytes));
    return {false, kInconsistentFileSize};
  }
  if (segment.m_expectedTotalBytes >= 0 && total != segment.m_expectedTotalBytes)
  {
    LOG(LWARNING, ("Content-Range total mismatch: got", total, "expected", segment.m_expectedTotalBytes));
    return {false, kInconsistentFileSize};
  }

  return {true, 206};
}

// static
string HttpClient::NormalizeServerCookies(string && cookies)
{
  std::istringstream is(cookies);
  string str, result;

  // Split by ", ". Can have invalid tokens here, expires= can also contain a comma.
  while (getline(is, str, ','))
  {
    size_t const leading = str.find_first_not_of(' ');
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
    result.append(str, 0, str.find(';'));
  }
  return result;
}

string DebugPrint(HttpClient const & request)
{
  std::ostringstream ostr;
  ostr << "HTTP " << request.ErrorCode() << " url [" << request.UrlRequested() << "]";
  if (request.WasRedirected())
    ostr << " was redirected to [" << request.UrlReceived() << "]";
  if (!request.ServerResponse().empty())
    ostr << " response: " << request.ServerResponse();
  return ostr.str();
}
}  // namespace platform
