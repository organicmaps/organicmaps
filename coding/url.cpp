#include "coding/url.hpp"
#include "coding/hex.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <sstream>
#include <vector>

namespace url
{
using namespace std;

std::string DebugPrint(Param const & param)
{
  return "UrlParam [" + param.m_name + "=" + param.m_value + "]";
}

Url::Url(std::string const & url)
{
  if (!Parse(url))
  {
    ASSERT(m_scheme.empty() && m_path.empty() && !IsValid(), ());
  }
}

Url Url::FromString(std::string const & url)
{
  bool const hasProtocol = strings::StartsWith(url, "http://") || strings::StartsWith(url, "https://");
  return Url(hasProtocol ? url : "https://" + url);
}

bool Url::Parse(std::string const & url)
{
  // Get url scheme.
  size_t pathStart = url.find(':');
  if (pathStart == string::npos || pathStart == 0)
    return false;
  m_scheme.assign(url, 0, pathStart);

  // Skip slashes.
  while (++pathStart < url.size() && url[pathStart] == '/')
  {
  }

  // Find query starting point for (key, value) parsing.
  size_t queryStart = url.find('?', pathStart);
  size_t pathLength;
  if (queryStart == string::npos)
  {
    queryStart = url.size();
    pathLength = queryStart - pathStart;
  }
  else
  {
    pathLength = queryStart - pathStart;
    ++queryStart;
  }

  // Get path (url without query).
  m_path.assign(url, pathStart, pathLength);

  // Parse query for keys and values.
  for (size_t start = queryStart; start < url.size();)
  {
    size_t end = url.find('&', start);
    if (end == string::npos)
      end = url.size();

    // Skip empty keys.
    if (end != start)
    {
      size_t const eq = url.find('=', start);

      string key;
      string value;
      if (eq != string::npos && eq < end)
      {
        key = UrlDecode(url.substr(start, eq - start));
        value = UrlDecode(url.substr(eq + 1, end - eq - 1));
      }
      else
      {
        key = UrlDecode(url.substr(start, end - start));
      }

      m_params.emplace_back(key, value);
    }

    start = end + 1;
  }

  return true;
}

string Url::GetWebDomain() const
{
  auto const found = m_path.find('/');
  if (found != string::npos)
    return m_path.substr(0, found);
  return m_path;
}

string Url::GetWebPath() const
{
  // Return everything after the domain name.
  auto const found = m_path.find('/');
  if (found != string::npos && m_path.size() > found + 1)
    return m_path.substr(found + 1);
  return {};
}

string Make(string const & baseUrl, Params const & params)
{
  ostringstream os;
  os << baseUrl;

  bool firstParam = baseUrl.find('?') == string::npos;
  for (auto const & param : params)
  {
    if (firstParam)
    {
      firstParam = false;
      os << "?";
    }
    else
    {
      os << "&";
    }

    os << param.m_name << "=" << param.m_value;
  }

  return os.str();
}

string Join(string const & lhs, string const & rhs)
{
  if (lhs.empty())
    return rhs;
  if (rhs.empty())
    return lhs;

  if (lhs.back() == '/' && rhs.front() == '/')
    return lhs + rhs.substr(1);

  if (lhs.back() != '/' && rhs.front() != '/')
    return lhs + '/' + rhs;

  return lhs + rhs;
}

string UrlEncode(string const & rawUrl)
{
  size_t const count = rawUrl.size();
  string result;
  result.reserve(count);

  for (size_t i = 0; i < count; ++i)
  {
    char const c = rawUrl[i];
    if (c < '-' || c == '/' || (c > '9' && c < 'A') || (c > 'Z' && c < '_') ||
        c == '`' || (c > 'z' && c < '~') || c > '~')
    {
      result += '%';
      result += NumToHex(c);
    }
    else
      result += rawUrl[i];
  }

  return result;
}

string UrlDecode(string const & encodedUrl)
{
  size_t const count = encodedUrl.size();
  string result;
  result.reserve(count);

  for (size_t i = 0; i < count; ++i)
  {
    if (encodedUrl[i] == '%')
    {
      result += FromHex(encodedUrl.substr(i + 1, 2));
      i += 2;
    }
    else
    {
      result += encodedUrl[i];
    }
  }

  return result;
}

}  // namespace url
