#pragma once

#include "base/base.hpp"

#include <functional>
#include <string>

namespace url_scheme
{
// Uri in format: 'scheme://path?key1=value1&key2&key3=&key4=value4'
class Uri
{
public:
  using Callback = std::function<bool(std::string const &, std::string const &)>;

  explicit Uri(std::string const & uri) : m_url(uri) { Init(); }
  Uri(char const * uri, size_t size) : m_url(uri, uri + size) { Init(); }

  std::string const & GetScheme() const { return m_scheme; }
  std::string const & GetPath() const { return m_path; }
  bool IsValid() const { return !m_scheme.empty(); }
  bool ForEachKeyValue(Callback const & callback) const;

private:
  void Init();
  bool Parse();

  std::string m_url;
  std::string m_scheme;
  std::string m_path;

  size_t m_queryStart;
};
}  // namespace url_scheme
