#pragma once

#include "geometry/rect2d.hpp"

#include "std/string.hpp"

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
  ;
};

class Uri;

enum class ParsingResult
{
  Incorrect,
  Map,
  Route
};

/// Handles [mapswithme|mwm]://map?params - everything related to displaying info on a map
class ParsedMapApi
{
public:
  ParsedMapApi();

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
  vector<RoutePoint> GetRoutePoints() const { return m_routePoints; }
  string GetRoutingType() const { return m_routingType; }
private:
  ParsingResult Parse(Uri const & uri);
  bool AddKeyValue(string key, string const & value, vector<ApiPoint> & points);
  bool RouteKeyValue(string key, string const & value, vector<string> & pattern);

  BookmarkManager * m_bmManager;
  vector<RoutePoint> m_routePoints;
  string m_globalBackUrl;
  string m_appTitle;
  string m_routingType;
  int m_version;
  /// Zoom level in OSM format (e.g. from 1.0 to 20.0)
  /// Taken into an account when calculating viewport rect, but only if points count is == 1
  double m_zoomLevel;
  bool m_goBackOnBalloonClick = false;
  bool m_isValid = false;
};

}
