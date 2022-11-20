#pragma once

#include "coding/url.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"
#include "geometry/latlon.hpp"

#include <string>
#include <vector>

class ApiMarkPoint;
class BookmarkManager;

namespace url
{
class Url;
}

namespace url_scheme
{
struct ApiPoint
{
  double m_lat;
  double m_lon;
  std::string m_name;
  std::string m_id;
  std::string m_style;
};

struct RoutePoint
{
  RoutePoint() = default;
  RoutePoint(m2::PointD const & org, std::string const & name) : m_org(org), m_name(name) {}
  m2::PointD m_org = m2::PointD::Zero();
  std::string m_name;
};

struct SearchRequest
{
  std::string m_query;
  std::string m_locale;
  bool m_isSearchOnMap = false;
};

/// Handles [mapswithme|mwm|mapsme]://map|route|search?params - everything related to displaying info on a map
class ParsedMapApi
{
public:
  enum class UrlType
  {
    Incorrect = 0,
    Map = 1,
    Route = 2,
    Search = 3,
    Crosshair = 4,
  };

  struct ParsingResult
  {
    UrlType m_type = UrlType::Incorrect;
    bool m_isSuccess = false;
  };

  ParsedMapApi() = default;

  void SetBookmarkManager(BookmarkManager * manager);

  ParsingResult SetUrlAndParse(std::string const & url);
  bool IsValid() const { return m_isValid; }
  std::string const & GetGlobalBackUrl() const { return m_globalBackUrl; }
  std::string const & GetAppName() const { return m_appName; }
  ms::LatLon GetCenterLatLon() const { return m_centerLatLon; }
  int GetApiVersion() const { return m_version; }
  void Reset();
  bool GoBackOnBalloonClick() const { return m_goBackOnBalloonClick; }

  /// @name Used in settings map viewport after invoking API.
  bool GetViewportParams(m2::PointD & center, double & scale) const;

  ApiMarkPoint const * GetSinglePoint() const;
  std::vector<RoutePoint> const & GetRoutePoints() const { return m_routePoints; }
  std::string const & GetRoutingType() const { return m_routingType; }
  SearchRequest const & GetSearchRequest() const { return m_searchRequest; }

private:
  /// Returns true when all statements are true:
  ///  - url parsed correctly;
  ///  - all mandatory parameters for url type |type| are provided;
  ///  - the order of params is correct (for UrlType::Map)
  bool Parse(url::Url const & url, UrlType type);

  void ParseMapParam(std::string const & key, std::string const & value,
                     std::vector<ApiPoint> & points, bool & correctOrder);
  void ParseRouteParam(std::string const & key, std::string const & value,
                       std::vector<std::string> & pattern);
  void ParseSearchParam(std::string const & key, std::string const & value);
  void ParseCommonParam(std::string const & key, std::string const & value);

  BookmarkManager * m_bmManager = nullptr;
  std::vector<RoutePoint> m_routePoints;
  SearchRequest m_searchRequest;
  std::string m_globalBackUrl;
  std::string m_appName;
  ms::LatLon m_centerLatLon;
  std::string m_routingType;
  int m_version = 0;
  /// Zoom level in OSM format (e.g. from 1.0 to 20.0)
  /// Taken into an account when calculating viewport rect, but only if points count is == 1
  double m_zoomLevel = 0.0;
  bool m_goBackOnBalloonClick = false;
  bool m_isValid = false;
};

std::string DebugPrint(ParsedMapApi::UrlType type);
}  // namespace url_scheme
