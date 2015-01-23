#pragma once

#include "api_mark_container.hpp"

#include "geometry/rect2d.hpp"

#include "std/string.hpp"

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

struct ParsedMapApi
{
  string m_globalBackUrl;
  string m_appTitle;
  int m_version = 0;
  /// Zoom level in OSM format (e.g. from 1.0 to 20.0)
  /// Taken into an account when calculating viewport rect, but only if points count is == 1
  double m_zoomLevel = 0.0;
  bool m_goBackOnBalloonClick = false;
  bool m_isValid = false;
};

/// Handles [mapswithme|mwm]://map?params - everything related to displaying info on a map
/// apiData - extracted data from url. If apiData.m_isValid == true then returned mark or rect is valid
/// If return nullptr and apiData.m_isValid == true, than more than one api point exists.
///   In rect returned bound rect of api points
/// If return not nullptr, than only one api point exists
UserMark const * ParseUrl(UserMarksController & controller, string const & url,
                          ParsedMapApi & apiData, m2::RectD & rect);

}
