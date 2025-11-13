#include "coding/url.hpp"
#include <string_view>
#include "coding/hex.hpp"

#include "base/assert.hpp"

namespace url
{
Url::Url(std::string_view url)
{
  if (!Parse(url))
    ASSERT(m_scheme.empty() && m_host.empty() && m_path.empty() && !IsValid(), ());
}

Url Url::FromString(std::string_view url)
{
  if (url.starts_with("http://") || url.starts_with("https://"))
    return Url(url);
  return Url("https://" + std::string(url));
}

bool Url::Parse(std::string_view url)
{
  static constexpr size_t kNotFound = std::string_view::npos;

  // Get url scheme.
  size_t start = url.find(':');
  if (start == kNotFound || start == 0)
    return false;
  m_scheme = url.substr(0, start);

  // Skip slashes.
  start = url.find_first_not_of('/', start + 1);
  if (start == kNotFound)
    return true;

  // Get host.
  size_t end = url.find_first_of("/?#", start);
  if (end == kNotFound)
  {
    m_host = url.substr(start);
    return true;
  }
  else
    m_host = url.substr(start, end - start);

  // Get path.
  if (url[end] == '/')
  {
    // Skip slashes.
    start = url.find_first_not_of('/', end);
    if (start == kNotFound)
      return true;

    end = url.find_first_of("?#", start);
    if (end == kNotFound)
    {
      m_path = url.substr(start);
      return true;
    }
    else
      m_path = url.substr(start, end - start);
  }

  // Parse query/fragment for keys and values.
  for (start = end + 1; start < url.size();)
  {
    end = url.find_first_of("&#", start);
    if (end == kNotFound)
      end = url.size();

    // Skip empty keys.
    if (end != start)
    {
      size_t const eq = url.find('=', start);

      std::string key, value;
      if (eq != kNotFound && eq < end)
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

std::string Join(std::string const & lhs, std::string const & rhs)
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

std::string UrlEncode(std::string_view component)
{
  size_t const count = component.size();
  std::string result;
  result.reserve(count);

  for (auto const c : component)
    if (c < '-' || c == '/' || (c > '9' && c < 'A') || (c > 'Z' && c < '_') || c == '`' || (c > 'z' && c < '~') ||
        c > '~')
    {
      result += '%';
      result += NumToHex(c);
    }
    else
      result += c;

  return result;
}

std::string UrlDecode(std::string_view encodedUrl)
{
  size_t const count = encodedUrl.size();
  std::string result;
  result.reserve(count);

  for (size_t i = 0; i < count; ++i)
  {
    if (encodedUrl[i] == '%')
    {
      result += FromHex(encodedUrl.substr(i + 1, 2));
      i += 2;
    }
    else if (encodedUrl[i] == '+')
    {
      result += ' ';
    }
    else
    {
      result += encodedUrl[i];
    }
  }

  return result;
}

}  // namespace url
