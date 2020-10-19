#pragma once

#include "coding/url.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

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
  double m_centerLat = 0.0;
  double m_centerLon = 0.0;
  bool m_isSearchOnMap = false;
};

struct Catalog
{
  std::string m_id;
  std::string m_name;
};

struct CatalogPath
{
  std::string m_url;
};

struct Subscription
{
  std::string m_groups;
};

namespace lead
{
struct CampaignDescription;
}

/// Handles [mapswithme|mwm|mapsme]://map|route|search?params - everything related to displaying info on a map
class ParsedMapApi
{
public:
  enum class UrlType
  {
    Incorrect,
    Map,
    Route,
    Search,
    Lead,
    Catalogue,
    CataloguePath,
    Subscription
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
  Catalog const & GetCatalog() const { return m_catalog; }
  CatalogPath const & GetCatalogPath() const { return m_catalogPath; }
  Subscription const & GetSubscription() const { return m_subscription; }
  std::string const & GetAffiliateId() const { return m_affiliateId; }

private:
  /// Returns true when all statements are true:
  ///  - url parsed correctly;
  ///  - all mandatory parameters for url type |type| are provided;
  ///  - the order of params is correct (for UrlType::Map)
  bool Parse(url::Url const & url, UrlType type);
  void ParseAdditional(url::Url const & url);
  void ParseMapParam(url::Param const & param, std::vector<ApiPoint> & points, bool & correctOrder);
  void ParseRouteParam(url::Param const & param, std::vector<std::string> & pattern);
  void ParseSearchParam(url::Param const & param, SearchRequest & request) const;
  void ParseLeadParam(url::Param const & param, lead::CampaignDescription & description) const;
  void ParseCatalogParam(url::Param const & param, Catalog & item) const;
  void ParseCatalogPathParam(url::Param const & param, CatalogPath & item) const;
  void ParseSubscriptionParam(url::Param const & param, Subscription & item) const;

  BookmarkManager * m_bmManager = nullptr;
  std::vector<RoutePoint> m_routePoints;
  SearchRequest m_request;
  Catalog m_catalog;
  CatalogPath m_catalogPath;
  Subscription m_subscription;
  std::string m_globalBackUrl;
  std::string m_appTitle;
  std::string m_routingType;
  std::string m_affiliateId;
  int m_version = 0;
  /// Zoom level in OSM format (e.g. from 1.0 to 20.0)
  /// Taken into an account when calculating viewport rect, but only if points count is == 1
  double m_zoomLevel = 0.0;
  bool m_goBackOnBalloonClick = false;
  bool m_isValid = false;
};

std::string DebugPrint(ParsedMapApi::UrlType type);
}  // namespace url_scheme
