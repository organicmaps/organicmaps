#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// Note. This should be "namespace coding::url" according to
// the style guide but we don't follow this convention here in
// order to simplify the usage.
namespace url
{

// Url in format: 'scheme://host/path?key1=value1&key2&key3=&key4=value4'
// host - any string ('omaps.app' or 'search'), without any valid domain check
class Url
{
public:
  explicit Url(std::string_view url);
  static Url FromString(std::string_view url);

  bool IsValid() const { return !m_scheme.empty(); }

  std::string const & GetScheme() const { return m_scheme; }
  std::string const & GetHost() const { return m_host; }
  std::string const & GetPath() const { return m_path; }
  std::string GetHostAndPath() const { return m_host + '/' + m_path; }

  template <class FnT>
  void ForEachParam(FnT && fn) const
  {
    for (auto const & p : m_params)
      fn(p.first, p.second);
  }

  using Param = std::pair<std::string, std::string>;
  Param const * GetLastParam() const { return m_params.empty() ? nullptr : &m_params.back(); }
  std::string const * GetParamValue(std::string const & name) const
  {
    for (auto const & p : m_params)
      if (p.first == name)
        return &p.second;
    return nullptr;
  }

private:
  bool Parse(std::string_view url);

  std::string m_scheme, m_host, m_path;
  std::vector<Param> m_params;
};

// Strict origin-only parser for untrusted urls, e.g. resource references in imported bookmark
// html descriptions. Parsing stops at the end of the authority, so path/query/fragment are never
// scanned and the work is bounded by the maximum authority length.
// Credentials, backslashes, percent-escapes, whitespace, control and non-ASCII bytes are rejected.
// Non-canonical IPv4 spellings that a browser would reinterpret are rejected too. IPv6 is outside
// this deliberately small safe subset. DNS names are restricted to conventional ASCII labels. The
// caller gets no origin for valid browser URLs outside the subset, never a wrong origin.
// Returns a normalized "http(s)://host[:port]" string. Default ports (80 for http, 443 for https)
// are omitted.
// allowProtocolRelative additionally accepts "//host/path" as an https origin.
std::optional<std::string> ParseHttpOrigin(std::string_view url, bool allowProtocolRelative);

// Joins URL, appends/removes slashes if needed.
std::string Join(std::string const & lhs, std::string const & rhs);

template <typename... Args>
std::string Join(std::string const & lhs, std::string const & rhs, Args &&... args)
{
  return Join(Join(lhs, rhs), std::forward<Args>(args)...);
}

std::string UrlEncode(std::string_view component);
std::string UrlDecode(std::string_view encodedComponent);

}  // namespace url
