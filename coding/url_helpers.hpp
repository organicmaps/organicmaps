#pragma once

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace coding::url
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

struct Param
{
  Param(std::string const & name, std::string const & value) : m_name(name), m_value(value) {}

  std::string m_name;
  std::string m_value;
};

using Params = std::vector<Param>;

// Make URL by using base url and vector of params.
std::string Make(std::string const & baseUrl, Params const & params);

// Joins URL, appends/removes slashes if needed.
std::string Join(std::string const & lhs, std::string const & rhs);

template <typename... Args>
std::string Join(std::string const & lhs, std::string const & rhs, Args &&... args)
{
  return Join(Join(lhs, rhs), std::forward<Args>(args)...);
}

std::string UrlEncode(std::string const & rawUrl);
std::string UrlDecode(std::string const & encodedUrl);

struct GeoURLInfo
{
  GeoURLInfo() { Reset(); }

  explicit GeoURLInfo(std::string const & s);

  bool IsValid() const;
  void Reset();

  void SetZoom(double x);
  bool SetLat(double x);
  bool SetLon(double x);

  double m_lat;
  double m_lon;
  double m_zoom;
};
}  // namespace coding::url
