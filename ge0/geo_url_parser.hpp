#pragma once

#include "coding/url.hpp"

#include <regex>
#include <string>

namespace geo
{

class GeoURLInfo
{
public:
  GeoURLInfo();

  bool IsLatLonValid() const;
  void Reset();

  void SetZoom(double x);
  bool SetLat(double x);
  bool SetLon(double x);

  double m_lat;
  double m_lon;
  double m_zoom;
  std::string m_query;
  std::string m_label;
};

class DoubleGISParser
{
public:
  DoubleGISParser();
  bool Parse(url::Url const & url, GeoURLInfo & info) const;

private:
  std::regex m_pathRe;
  std::regex m_paramRe;
};

class OpenStreetMapParser
{
public:
  OpenStreetMapParser();
  bool Parse(url::Url const & url, GeoURLInfo & info) const;

private:
  std::regex m_regex;
};

class LatLonParser
{
public:
  LatLonParser();
  void Reset(url::Url const & url, GeoURLInfo & info);

  bool IsValid() const;
  void operator()(std::string name, std::string const & value);

private:
  // Usually (lat, lon), but some providers use (lon, lat).
  static int constexpr kLLPriority = 5;
  // We do not try to guess the projection and do not interpret (x, y)
  // as Mercator coordinates in URLs. We simply use (y, x) for (lat, lon).
  static int constexpr kXYPriority = 6;
  static int constexpr kLatLonPriority = 7;

  // Priority for accepting coordinates if we have many choices.
  // -1 - not initialized
  //  0 - coordinates in path;
  //  x - priority for query type (greater is better)
  static int GetCoordinatesPriority(std::string const & token);

  GeoURLInfo * m_info;
  bool m_swapLatLon;
  std::regex m_regexp;
  int m_latPriority;
  int m_lonPriority;
};

class GeoParser
{
public:
  GeoParser();
  bool Parse(std::string const & url, GeoURLInfo & info) const;

private:
  std::regex m_latlonRe;
  std::regex m_zoomRe;
};

class UnifiedParser
{
public:
  bool Parse(std::string const & url, GeoURLInfo & info);

private:
  GeoParser m_geoParser;
  DoubleGISParser m_dgParser;
  OpenStreetMapParser m_osmParser;
  LatLonParser m_llParser;
};

}  // namespace geo
