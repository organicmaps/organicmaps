#pragma once

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include <string>
#include <vector>

class ApiMarkPoint;
class BookmarkManager;

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
  double m_centerLat = 0.0;
  double m_centerLon = 0.0;
  bool m_isSearchOnMap = false;
};

struct CatalogItem
{
  std::string m_id;
  std::string m_name;
};

struct CatalogPathItem
{
  std::string m_url;
};

namespace lead
{
struct CampaignDescription;
}

class Uri;

/// Handles [mapswithme|mwm|mapsme]://map|route|search?params - everything related to displaying info on a map
class ParsedMapApi
{
public:
  enum class ParsingResult
  {
    Incorrect,
    Map,
    Route,
    Search,
    Lead,
    Catalogue,
    CataloguePath
  };

  ParsedMapApi() = default;

  void SetBookmarkManager(BookmarkManager * manager);

  ParsingResult SetUriAndParse(std::string const & url);
  bool IsValid() const { return m_isValid; }
  std::string const & GetGlobalBackUrl() const { return m_globalBackUrl; }
  std::string const & GetAppTitle() const { return m_appTitle; }
  int GetApiVersion() const { return m_version; }
  void Reset();
  bool GoBackOnBalloonClick() const { return m_goBackOnBalloonClick; }

  /// @name Used in settings map viewport after invoking API.
  bool GetViewportRect(m2::RectD & rect) const;
  ApiMarkPoint const * GetSinglePoint() const;
  std::vector<RoutePoint> const & GetRoutePoints() const { return m_routePoints; }
  std::string const & GetRoutingType() const { return m_routingType; }
  SearchRequest const & GetSearchRequest() const { return m_request; }
  CatalogItem const & GetCatalogItem() const { return m_catalogItem; }
  CatalogPathItem const & GetCatalogPathItem() const { return m_catalogPathItem; }
private:
  ParsingResult Parse(Uri const & uri);
  bool AddKeyValue(std::string const & key, std::string const & value, std::vector<ApiPoint> & points);
  bool RouteKeyValue(std::string const & key, std::string const & value, std::vector<std::string> & pattern);
  bool SearchKeyValue(std::string const & key, std::string const & value, SearchRequest & request) const;
  bool LeadKeyValue(std::string const & key, std::string const & value, lead::CampaignDescription & description) const;
  bool CatalogKeyValue(std::string const & key, std::string const & value, CatalogItem & item) const;
  bool CatalogPathKeyValue(std::string const & key, std::string const & value, CatalogPathItem & item) const;

  BookmarkManager * m_bmManager = nullptr;
  std::vector<RoutePoint> m_routePoints;
  SearchRequest m_request;
  CatalogItem m_catalogItem;
  CatalogPathItem m_catalogPathItem;
  std::string m_globalBackUrl;
  std::string m_appTitle;
  std::string m_routingType;
  int m_version = 0;
  /// Zoom level in OSM format (e.g. from 1.0 to 20.0)
  /// Taken into an account when calculating viewport rect, but only if points count is == 1
  double m_zoomLevel = 0.0;
  bool m_goBackOnBalloonClick = false;
  bool m_isValid = false;
};
}  // namespace url_scheme
