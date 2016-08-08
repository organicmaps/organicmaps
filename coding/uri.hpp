#pragma once

#include "base/base.hpp"

#include "std/function.hpp"
#include "std/string.hpp"


namespace url_scheme
{

// Uri in format: 'scheme://path?key1=value1&key2&key3=&key4=value4'
class Uri
{
public:
  using TCallback = function<bool(string const &, string const &)>;

  explicit Uri(string const & uri) : m_url(uri) { Init(); }
  Uri(char const * uri, size_t size) : m_url(uri, uri + size) { Init(); }

  string const & GetScheme() const { return m_scheme; }
  string const & GetPath() const { return m_path; }
  bool IsValid() const { return !m_scheme.empty(); }
  bool ForEachKeyValue(TCallback const & callback) const;

private:
  void Init();
  bool Parse();

  string m_url;
  string m_scheme;
  string m_path;

  size_t m_queryStart;
};

}  // namespace url_scheme
