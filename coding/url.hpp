#pragma once

#include <functional>
#include <string>
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
  explicit Url(std::string const & url);
  static Url FromString(std::string const & url);

  bool IsValid() const { return !m_scheme.empty(); }

  std::string const & GetScheme() const { return m_scheme; }
  std::string const & GetHost() const { return m_host; }
  std::string const & GetPath() const { return m_path; }
  std::string GetHostAndPath() const { return m_host + '/' + m_path; }

  template <class FnT> void ForEachParam(FnT && fn) const
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
  bool Parse(std::string const & url);

  std::string m_scheme, m_host, m_path;
  std::vector<Param> m_params;
};

// Joins URL, appends/removes slashes if needed.
std::string Join(std::string const & lhs, std::string const & rhs);

template <typename... Args>
std::string Join(std::string const & lhs, std::string const & rhs, Args &&... args)
{
  return Join(Join(lhs, rhs), std::forward<Args>(args)...);
}

// Replaces all special characters with '-'
std::string Slug(std::string const & raw);

std::string UrlEncode(std::string const & rawUrl);
std::string UrlDecode(std::string_view encodedUrl);

}  // namespace url
