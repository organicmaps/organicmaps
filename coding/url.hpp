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
struct Param
{
  Param(std::string const & name, std::string const & value) : m_name(name), m_value(value) {}

  std::string m_name;
  std::string m_value;
};

std::string DebugPrint(Param const & param);

using Params = std::vector<Param>;

// Url in format: 'scheme://path?key1=value1&key2&key3=&key4=value4'
class Url
{
public:
  using Callback = std::function<void(Param const & param)>;

  explicit Url(std::string const & url);

  std::string const & GetScheme() const { return m_scheme; }
  std::string const & GetPath() const { return m_path; }
  bool IsValid() const { return !m_scheme.empty(); }
  void ForEachParam(Callback const & callback) const;

private:
  bool Parse(std::string const & url);

  std::string m_scheme;
  std::string m_path;
  std::vector<Param> m_params;
};

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
}  // namespace url
