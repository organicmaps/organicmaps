#include "map/mwm_url.hpp"

#include "map/api_mark_point.hpp"
#include "map/bookmark_manager.hpp"
#include "map/framework.hpp"

#include "ge0/parser.hpp"
#include "ge0/geo_url_parser.hpp"

#include "geometry/mercator.hpp"
#include "geometry/latlon.hpp"
#include "indexer/scales.hpp"

#include "drape_frontend/visual_params.hpp"

#include "base/logging.hpp"
#include "base/scope_guard.hpp"
#include "base/string_utils.hpp"

#include <array>
#include <tuple>

namespace url_scheme
{
using namespace std;

namespace
{
string_view constexpr kCenterLatLon = "cll";
string_view constexpr kAppName = "appname";

namespace map
{
char const * kLatLon = "ll";
char const * kZoomLevel = "z";
char const * kName = "n";
char const * kId = "id";
char const * kStyle = "s";
char const * kBackUrl = "backurl";
char const * kVersion = "v";
char const * kBalloonAction = "balloonaction";
}  // namespace map

namespace route
{
char const * kSourceLatLon = "sll";
char const * kDestLatLon = "dll";
char const * kSourceName = "saddr";
char const * kDestName = "daddr";
char const * kRouteType = "type";
char const * kRouteTypeVehicle = "vehicle";
char const * kRouteTypePedestrian = "pedestrian";
char const * kRouteTypeBicycle = "bicycle";
char const * kRouteTypeTransit = "transit";
}  // namespace route

namespace search
{
char const * kQuery = "query";
char const * kLocale = "locale";
char const * kSearchOnMap = "map";
}  // namespace search

// See also kGe0Prefixes in ge0/parser.hpp
std::array<std::string_view, 3> const kLegacyMwmPrefixes = {{
  "mapsme://", "mwm://", "mapswithme://"
}};

bool ParseLatLon(std::string const & key, std::string const & value,
                 double & lat, double & lon)
{
  size_t const firstComma = value.find(',');
  if (firstComma == string::npos)
  {
    LOG(LWARNING, ("Map API: no comma between lat and lon for key:", key, " value:", value));
    return false;
  }

  if (!strings::to_double(value.substr(0, firstComma), lat) ||
      !strings::to_double(value.substr(firstComma + 1), lon))
  {
    LOG(LWARNING, ("Map API: can't parse lat,lon for key:", key, " value:", value));
    return false;
  }

  if (!mercator::ValidLat(lat) || !mercator::ValidLon(lon))
  {
    LOG(LWARNING, ("Map API: incorrect value for lat and/or lon", key, value, lat, lon));
    return false;
  }
  return true;
}

// A helper for SetUrlAndParse() below.
// kGe0Prefixes supports API and ge0 links, kLegacyMwmPrefixes - only API.
inline tuple<size_t, bool> findUrlPrefix(string const & url)
{
  for (auto prefix : ge0::kGe0Prefixes)
  {
    if (strings::StartsWith(url, prefix))
      return {prefix.size(), true};
  }
  for (auto prefix : kLegacyMwmPrefixes)
  {
    if (strings::StartsWith(url, prefix))
      return {prefix.size(), false};
  }
  return {string::npos, false};
}
}  // namespace

ParsedMapApi::UrlType ParsedMapApi::SetUrlAndParse(string const & raw)
{
  Reset();
  SCOPE_GUARD(guard, [this]() { Reset(); });

  auto [prefix, checkForGe0Link] = findUrlPrefix(raw);
  if (prefix != string::npos)
  {
    url::Url const url = url::Url("om://" + raw.substr(prefix));
    if (!url.IsValid())
      return m_requestType = UrlType::Incorrect;

    // The URL is prefixed by one of the kGe0Prefixes or kLegacyMwmPrefixes prefixes
    // => check for well-known API methods first.
    string const & type = url.GetHost();
    if (type == "map")
    {
      bool correctOrder = true;
      url.ForEachParam([&correctOrder, this](auto const & key, auto const & value)
      {
        ParseMapParam(key, value, correctOrder);
      });

      if (m_mapPoints.empty() || !correctOrder)
        return m_requestType = UrlType::Incorrect;

      guard.release();
      return m_requestType = UrlType::Map;
    }
    else if (type == "route")
    {
      m_routePoints.clear();
      using namespace route;
      vector<string> pattern{kSourceLatLon, kSourceName, kDestLatLon, kDestName, kRouteType};
      url.ForEachParam([&pattern, this](auto const & key, auto const & value)
      {
        ParseRouteParam(key, value, pattern);
      });

      if (pattern.size() != 0)
        return m_requestType = UrlType::Incorrect;

      if (m_routePoints.size() != 2)
        return m_requestType = UrlType::Incorrect;

      guard.release();
      return m_requestType = UrlType::Route;
    }
    else if (type == "search")
    {
      url.ForEachParam([this](auto const & key, auto const & value)
      {
        ParseSearchParam(key, value);
      });
      if (m_searchRequest.m_query.empty())
        return m_requestType = UrlType::Incorrect;

      guard.release();
      return m_requestType = UrlType::Search;
    }
    else if (type == "crosshair")
    {
      url.ForEachParam([this](auto const & key, auto const & value)
      {
         ParseCommonParam(key, value);
      });

      guard.release();
      return m_requestType = UrlType::Crosshair;
    }
    else if (checkForGe0Link)
    {
      // The URL is prefixed by one of the kGe0Prefixes AND doesn't match any supported API call:
      // => try to decode ge0 short link.
      ge0::Ge0Parser parser;
      ge0::Ge0Parser::Result info;
      if (!parser.ParseAfterPrefix(raw, prefix, info))
        return UrlType::Incorrect;

      m_zoomLevel = info.m_zoomLevel;
      m_mapPoints.push_back({info.m_lat, info.m_lon, info.m_name, "" /* m_id */, "" /* m_style */});

      guard.release();
      return m_requestType = UrlType::Map;
    }
    else
    {
      // The URL is prefixed by one of the kLegacyPrefixes AND doesn't match any supported API call => error.
      LOG(LWARNING, ("Unsupported legacy API URL:", raw));
      return m_requestType = UrlType::Incorrect;
    }
  }
  else
  {
    // The URL is not prefixed by any special prefixes => try to parse for lat,lon and other geo parameters.
    geo::GeoURLInfo info;
    if (!geo::UnifiedParser().Parse(raw, info))
      return m_requestType = UrlType::Incorrect;

    if (!info.m_query.empty())
    {
      // The URL has "q=" parameter => convert to a Search API request.
      m_searchRequest = SearchRequest();
      m_searchRequest.m_query = info.m_query;
      m_centerLatLon = {info.m_lat, info.m_lon};
      m_zoomLevel = info.m_zoom;
      guard.release();
      return m_requestType = UrlType::Search;
    }
    else
    {
     // The URL doesn't have "q=" parameter => convert to a Map API request.
      ASSERT(info.IsLatLonValid(), ());
      m_centerLatLon = {info.m_lat, info.m_lon};
      m_zoomLevel = info.m_zoom;
      m_mapPoints.push_back({info.m_lat, info.m_lon, info.m_label, "" /* m_id */, "" /* m_style */});
      guard.release();
      return m_requestType = UrlType::Map;
    }
  }
  UNREACHABLE();
}

void ParsedMapApi::ParseMapParam(std::string const & key, std::string const & value,
                                 bool & correctOrder)
{
  using namespace map;

  if (key == kLatLon)
  {
    double lat = 0.0;
    double lon = 0.0;
    if (!ParseLatLon(key, value, lat, lon))
    {
      LOG(LWARNING, ("Incorrect 'll':", value));
      return;
    }

    m_mapPoints.push_back({lat, lon, "" /* m_name */, "" /* m_id */, "" /* m_style */});
  }
  else if (key == kZoomLevel)
  {
    double zoomLevel;
    if (strings::to_double(value, zoomLevel))
      m_zoomLevel = zoomLevel;
  }
  else if (key == kName)
  {
    if (!m_mapPoints.empty())
    {
      m_mapPoints.back().m_name = value;
    }
    else
    {
      LOG(LWARNING, ("Map API: Point name with no point. 'll' should come first!"));
      correctOrder = false;
      return;
    }
  }
  else if (key == kId)
  {
    if (!m_mapPoints.empty())
    {
      m_mapPoints.back().m_id = value;
    }
    else
    {
      LOG(LWARNING, ("Map API: Point url with no point. 'll' should come first!"));
      correctOrder = false;
      return;
    }
  }
  else if (key == kStyle)
  {
    if (!m_mapPoints.empty())
    {
      m_mapPoints.back().m_style = value;
    }
    else
    {
      LOG(LWARNING, ("Map API: Point style with no point. 'll' should come first!"));
      return;
    }
  }
  else if (key == kBackUrl)
  {
    // Fix missing :// in back url, it's important for iOS
    if (value.find("://") == string::npos)
      m_globalBackUrl = value + "://";
    else
      m_globalBackUrl = value;
  }
  else if (key == kVersion)
  {
    if (!strings::to_int(value, m_version))
      m_version = 0;
  }
  else if (key == kBalloonAction)
  {
    m_goBackOnBalloonClick = true;
  }
  else
  {
    ParseCommonParam(key, value);
  }
}

void ParsedMapApi::ParseRouteParam(std::string const & key, std::string const & value,
                                   vector<string> & pattern)
{
  using namespace route;

  if (pattern.empty() || key != pattern.front())
    return;

  if (key == kSourceLatLon || key == kDestLatLon)
  {
    double lat = 0.0;
    double lon = 0.0;
    if (!ParseLatLon(key, value, lat, lon))
    {
      LOG(LWARNING, ("Incorrect 'sll':", value));
      return;
    }

    RoutePoint p;
    p.m_org = mercator::FromLatLon(lat, lon);
    m_routePoints.push_back(p);
  }
  else if (key == kSourceName || key == kDestName)
  {
    m_routePoints.back().m_name = value;
  }
  else if (key == kRouteType)
  {
    string const lowerValue = strings::MakeLowerCase(value);
    if (lowerValue == kRouteTypePedestrian || lowerValue == kRouteTypeVehicle ||
        lowerValue == kRouteTypeBicycle || lowerValue == kRouteTypeTransit)
    {
      m_routingType = lowerValue;
    }
    else
    {
      LOG(LWARNING, ("Incorrect routing type:", value));
      return;
    }
  }

  pattern.erase(pattern.begin());
}

void ParsedMapApi::ParseSearchParam(std::string const & key, std::string const & value)
{
  using namespace search;

  if (key == kQuery)
  {
    m_searchRequest.m_query = value;
  }
  else if (key == kLocale)
  {
    m_searchRequest.m_locale = value;
  }
  else if (key == kSearchOnMap)
  {
    m_searchRequest.m_isSearchOnMap = true;
  }
  else
  {
    ParseCommonParam(key, value);
  }
}

void ParsedMapApi::ParseCommonParam(std::string const & key, std::string const & value)
{
  if (key == kAppName)
  {
    m_appName = value;
  }
  else if (key == kCenterLatLon)
  {
    double lat, lon;
    if (ParseLatLon(key, value, lat, lon))
      m_centerLatLon = {lat, lon};
    else
      LOG(LWARNING, ("Incorrect 'cll':", value));
  }
}

void ParsedMapApi::Reset()
{
  m_requestType = UrlType::Incorrect;
  m_mapPoints = {};
  m_routePoints = {};
  m_searchRequest = {};
  m_globalBackUrl ={};
  m_appName = {};
  m_centerLatLon = ms::LatLon::Invalid();
  m_routingType = {};
  m_version = 0;
  m_zoomLevel = 0.0;
  m_goBackOnBalloonClick = false;
}

void ParsedMapApi::ExecuteMapApiRequest(Framework & fm)
{
  ASSERT_EQUAL(m_requestType, UrlType::Map, ("Must be a Map API request"));
  VERIFY(m_mapPoints.size() > 0, ("Map API request must have at least one point"));

  // Clear every current API-mark.
  auto editSession = fm.GetBookmarkManager().GetEditSession();
  editSession.ClearGroup(UserMark::Type::API);
  editSession.SetIsVisible(UserMark::Type::API, true);

  // Add marks from the request.
  m2::RectD viewport;
  for (auto const & p : m_mapPoints)
  {
    m2::PointD glPoint(mercator::FromLatLon(p.m_lat, p.m_lon));
    auto * mark = editSession.CreateUserMark<ApiMarkPoint>(glPoint);
    mark->SetName(p.m_name);
    mark->SetApiID(p.m_id);
    mark->SetStyle(style::GetSupportedStyle(p.m_style));
    viewport.Add(glPoint);
  }

  // Calculate the optimal viewport.
  VERIFY(viewport.IsValid(), ());
  m2::PointD const center = viewport.Center();

  // Calculate the best zoom.
  int zoomLevel;
  if (m_mapPoints.size() == 1)
  {
    if (m_zoomLevel >= 1.0)  // 0 means uninitialized/not passed to the API.
      zoomLevel = min(scales::GetUpperComfortScale(), static_cast<int>(m_zoomLevel));
    else
      zoomLevel = scales::GetUpperComfortScale();
  }
  else
  {
    zoomLevel = df::GetDrawTileScale(viewport);
  }

  // Always hide current map selection.
  fm.DeactivateMapSelection(true /* notifyUI */);

  // Set viewport and stop follow mode.
  fm.StopLocationFollow();

  // ShowRect function interferes with ActivateMapSelection and we have strange behaviour as a result.
  // Use more obvious SetModelViewCenter here.
  fm.SetViewportCenter(center, zoomLevel, true);

  // Don't show the place page in case of multiple points.
  if (m_mapPoints.size() > 1)
    return;
  place_page::BuildInfo info;
  info.m_needAnimationOnSelection = false;
  info.m_mercator = mercator::FromLatLon(m_mapPoints[0].m_lat, m_mapPoints[0].m_lon);
  // Other details will be filled in by BuildPlacePageInfo().
  fm.BuildAndSetPlacePageInfo(info);
}

std::string DebugPrint(ParsedMapApi::UrlType type)
{
  switch(type)
  {
  case ParsedMapApi::UrlType::Incorrect: return "Incorrect";
  case ParsedMapApi::UrlType::Map: return "Map";
  case ParsedMapApi::UrlType::Route: return "Route";
  case ParsedMapApi::UrlType::Search: return "Search";
  case ParsedMapApi::UrlType::Crosshair: return "Crosshair";
  }
  UNREACHABLE();
}
}  // namespace url_scheme
