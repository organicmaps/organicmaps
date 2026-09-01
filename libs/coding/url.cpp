#include "coding/url.hpp"

#include "coding/hex.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

#include <cstdint>
#include <string_view>

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

namespace
{
// An origin is ASCII-only by design, see ParseHttpOrigin().
size_t constexpr kMaxHostSize = 253;
size_t constexpr kMaxLabelSize = 63;
size_t constexpr kMaxPortDigits = 5;

// Rejects credentials ('@'), backslashes, percent-escapes, whitespace, control and non-ASCII bytes.
bool IsSafeAuthorityChar(char c)
{
  auto const byte = static_cast<unsigned char>(c);
  return byte > 0x20 && byte < 0x7F && byte != '%' && byte != '\\' && byte != '@';
}

// Lowercase conventional DNS name or IPv4 literal: [a-z0-9.-] with non-empty labels.
bool IsValidDnsHost(std::string_view host)
{
  if (host.empty() || host.size() > kMaxHostSize)
    return false;

  size_t labelSize = 0;
  for (char const c : host)
  {
    if (c == '.')
    {
      if (labelSize == 0)  // Leading or double dot.
        return false;
      labelSize = 0;
      continue;
    }

    if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-'))
      return false;
    if (++labelSize > kMaxLabelSize)
      return false;
  }

  return labelSize != 0;  // Trailing dot.
}

bool IsDecimal(std::string_view s)
{
  if (s.empty())
    return false;
  for (char const c : s)
    if (c < '0' || c > '9')
      return false;
  return true;
}

// The browser URL parser treats a host whose last label is a number as IPv4, including legacy
// one-part, octal and hexadecimal spellings. Such a host must either be canonical IPv4 or fail.
bool EndsInIPv4Number(std::string_view host)
{
  size_t const dot = host.rfind('.');
  std::string_view const last = host.substr(dot == std::string_view::npos ? 0 : dot + 1);
  if (IsDecimal(last))
    return true;
  if (!last.starts_with("0x"))
    return false;
  for (char const c : last.substr(2))
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
      return false;
  return true;
}

bool IsCanonicalIPv4(std::string_view host)
{
  size_t pos = 0;
  for (size_t part = 0; part < 4; ++part)
  {
    size_t const end = host.find('.', pos);
    if ((part < 3) != (end != std::string_view::npos))
      return false;

    std::string_view const digits = host.substr(pos, end - pos);
    if (!IsDecimal(digits) || digits.size() > 3 || (digits.size() > 1 && digits.front() == '0'))
      return false;

    uint32_t value = 0;
    for (char const c : digits)
      value = value * 10 + static_cast<uint32_t>(c - '0');
    if (value > 255)
      return false;

    if (part < 3)
      pos = end + 1;
  }
  return true;
}
}  // namespace

std::optional<std::string> ParseHttpOrigin(std::string_view url, bool allowProtocolRelative)
{
  std::string_view scheme;
  size_t authorityPos;
  if (strings::EqualAsciiNoCase(url.substr(0, 8), "https://"))
  {
    scheme = "https";
    authorityPos = 8;
  }
  else if (strings::EqualAsciiNoCase(url.substr(0, 7), "http://"))
  {
    scheme = "http";
    authorityPos = 7;
  }
  else if (allowProtocolRelative && url.starts_with("//"))
  {
    scheme = "https";
    authorityPos = 2;
  }
  else
    return {};

  // The authority ends at the path/query/fragment delimiter and is never longer than
  // host + ':' + port, so the rest of the url (e.g. a huge query) is not scanned at all.
  size_t constexpr kMaxAuthoritySize = kMaxHostSize + 1 + kMaxPortDigits;
  std::string_view authority = url.substr(authorityPos, kMaxAuthoritySize + 1);
  if (size_t const end = authority.find_first_of("/?#"); end != std::string_view::npos)
    authority = authority.substr(0, end);
  else if (authority.size() > kMaxAuthoritySize)
    return {};

  for (char const c : authority)
    if (!IsSafeAuthorityChar(c))
      return {};

  // IPv6 needs full parsing and canonical serialization to represent a browser origin correctly.
  // It is outside this deliberately small safe subset.
  if (authority.starts_with('['))
    return {};

  // Split the authority into the host and the optional port.
  size_t const colon = authority.find(':');
  std::string_view host = authority;
  uint32_t port = 0;
  if (colon != std::string_view::npos)
  {
    host = authority.substr(0, colon);

    std::string_view const portDigits = authority.substr(colon + 1);
    if (portDigits.empty() || portDigits.size() > kMaxPortDigits)
      return {};

    uint32_t value = 0;
    for (char const c : portDigits)
    {
      if (c < '0' || c > '9')
        return {};
      value = value * 10 + static_cast<uint32_t>(c - '0');
    }

    if (value == 0 || value > 0xFFFF)
      return {};

    // A default port is not a part of the origin.
    if (!((scheme == "http" && value == 80) || (scheme == "https" && value == 443)))
      port = value;
  }

  std::string origin;
  origin.reserve(scheme.size() + 3 + host.size() + (port == 0 ? 0 : 6));
  origin.append(scheme).append("://").append(host);
  strings::AsciiToLower(origin);

  std::string_view const normalizedHost(origin.data() + scheme.size() + 3, host.size());
  if (!IsValidDnsHost(normalizedHost) || (EndsInIPv4Number(normalizedHost) && !IsCanonicalIPv4(normalizedHost)))
    return {};

  if (port != 0)
    origin.append(1, ':').append(std::to_string(port));
  return origin;
}

}  // namespace url
