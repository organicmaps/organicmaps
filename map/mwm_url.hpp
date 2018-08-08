#pragma once

#include "geometry/rect2d.hpp"

#include "std/string.hpp"
#include "std/vector.hpp"

class ApiMarkPoint;
class BookmarkManager;

namespace url_scheme
{

struct ApiPoint
{
  double m_lat;
  double m_lon;
  string m_name;
  string m_id;
  string m_style;
};

struct RoutePoint
{
  RoutePoint() = default;
  RoutePoint(m2::PointD const & org, string const & name) : m_org(org), m_name(name) {}
  m2::PointD m_org = m2::PointD::Zero();
  string m_name;
};

struct SearchRequest
{
  string m_query;
  string m_locale;
  double m_centerLat = 0.0;
  double m_centerLon = 0.0;
  bool m_isSearchOnMap = false;
};

struct CatalogItem
{
  string m_id;
  string m_name;
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
    Catalogue
  };

  ParsedMapApi() = default;

  void SetBookmarkManager(BookmarkManager * manager);

  ParsingResult SetUriAndParse(string const & url);
  bool IsValid() const { return m_isValid; }
  string const & GetGlobalBackUrl() const { return m_globalBackUrl; }
  string const & GetAppTitle() const { return m_appTitle; }
  int GetApiVersion() const { return m_version; }
  void Reset();
  bool GoBackOnBalloonClick() const { return m_goBackOnBalloonClick; }

  /// @name Used in settings map viewport after invoking API.
  bool GetViewportRect(m2::RectD & rect) const;
  ApiMarkPoint const * GetSinglePoint() const;
  vector<RoutePoint> const & GetRoutePoints() const { return m_routePoints; }
  string const & GetRoutingType() const { return m_routingType; }
  SearchRequest const & GetSearchRequest() const { return m_request; }
  CatalogItem const & GetCatalogItem() const { return m_catalogItem; }
private:
  ParsingResult Parse(Uri const & uri);
  bool AddKeyValue(string const & key, string const & value, vector<ApiPoint> & points);
  bool RouteKeyValue(string const & key, string const & value, vector<string> & pattern);
  bool SearchKeyValue(string const & key, string const & value, SearchRequest & request) const;
  bool LeadKeyValue(string const & key, string const & value, lead::CampaignDescription & description) const;
  bool CatalogKeyValue(string const & key, string const & value, CatalogItem & item) const;

  BookmarkManager * m_bmManager = nullptr;
  vector<RoutePoint> m_routePoints;
  SearchRequest m_request;
  CatalogItem m_catalogItem;
  string m_globalBackUrl;
  string m_appTitle;
  string m_routingType;
  int m_version = 0;
  /// Zoom level in OSM format (e.g. from 1.0 to 20.0)
  /// Taken into an account when calculating viewport rect, but only if points count is == 1
  double m_zoomLevel = 0.0;
  bool m_goBackOnBalloonClick = false;
  bool m_isValid = false;
};

}
