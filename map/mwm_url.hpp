#pragma once

#include "map/user_mark.hpp"

#include "geometry/rect2d.hpp"

#include "std/string.hpp"

class ScalesProcessor;
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

class Uri;

/// Handles [mapswithme|mwm]://map?params - everything related to displaying info on a map
class ParsedMapApi
{
public:
  ParsedMapApi();

  void SetBookmarkManager(BookmarkManager * manager);

  bool SetUriAndParse(string const & url);
  bool IsValid() const;
  string const & GetGlobalBackUrl() const { return m_globalBackUrl; }
  string const & GetAppTitle() const { return m_appTitle; }
  int GetApiVersion() const { return m_version; }
  void Reset();
  bool GoBackOnBalloonClick() const { return m_goBackOnBalloonClick; }

  /// @name Used in settings map viewport after invoking API.
  bool GetViewportRect(m2::RectD & rect) const;
  UserMark const * GetSinglePoint() const;

private:
  bool Parse(Uri const & uri);
  void AddKeyValue(string key, string const & value, vector<ApiPoint> & points);

  BookmarkManager * m_bmManager;
  string m_globalBackUrl;
  string m_appTitle;
  int m_version;
  /// Zoom level in OSM format (e.g. from 1.0 to 20.0)
  /// Taken into an account when calculating viewport rect, but only if points count is == 1
  double m_zoomLevel;
  bool m_goBackOnBalloonClick;
};

}
