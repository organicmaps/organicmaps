#include "map/mwm_url.hpp"

#include "map/api_mark_point.hpp"
#include "map/bookmark_manager.hpp"
#include "map/framework.hpp"
#include "map/routing_mark.hpp"

#include "ge0/geo_url_parser.hpp"
#include "ge0/parser.hpp"

#include "coding/url.hpp"

#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"
#include "indexer/scales.hpp"

#include "drape_frontend/visual_params.hpp"

#include "base/logging.hpp"
#include "base/math.hpp"
#include "base/scope_guard.hpp"
#include "base/string_utils.hpp"

#include <array>
#include <cmath>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace url_scheme
{
namespace
{
std::string_view constexpr kCenterLatLon = "cll";
std::string_view constexpr kAppName = "appname";
std::string_view constexpr kVersion = "v";

// Zoom of a shared clear-coordinate link (omaps.app/<lat>,<lon>), taken from "?z=". Mirrors
// url-processor's normalizeZoom: an integer in [1, kMaxClearCoordinatesZoom], anything else
// (missing, malformed or out of range) falls back to the default.
int constexpr kDefaultClearCoordinatesZoom = 14;
uint32_t constexpr kMaxClearCoordinatesZoom = scales::UPPER_STYLE_SCALE;

namespace map
{
std::string_view constexpr kLatLon = "ll";
std::string_view constexpr kZoomLevel = "z";
std::string_view constexpr kName = "n";
std::string_view constexpr kId = "id";
std::string_view constexpr kStyle = "s";
std::string_view constexpr kBackUrl = "backurl";
std::string_view constexpr kBalloonAction = "balloonaction";
}  // namespace map

namespace route
{
std::string_view constexpr kSourceLatLon = "sll";
std::string_view constexpr kDestLatLon = "dll";
std::string_view constexpr kSourceName = "saddr";
std::string_view constexpr kDestName = "daddr";
std::string_view constexpr kRouteType = "type";
std::string_view constexpr kRouteTypeVehicle = "vehicle";
std::string_view constexpr kRouteTypePedestrian = "pedestrian";
std::string_view constexpr kRouteTypeBicycle = "bicycle";
std::string_view constexpr kRouteTypeTransit = "transit";
}  // namespace route

namespace route_v2
{
std::string_view constexpr kPathDir = "dir";
std::string_view constexpr kPathNav = "nav";
std::string_view constexpr kOrigin = "origin";
std::string_view constexpr kOriginCurrentLocation = "currentLocation";
std::string_view constexpr kOriginCurrentLocationKebab = "current-location";
std::string_view constexpr kOriginName = "origin_name";
std::string_view constexpr kOriginCallback = "origin_callback";
std::string_view constexpr kOriginHeading = "origin_heading";
std::string_view constexpr kDestination = "destination";
std::string_view constexpr kDestinationName = "destination_name";
std::string_view constexpr kDestinationCallback = "destination_callback";
std::string_view constexpr kWaypoints = "waypoints";
std::string_view constexpr kWaypointNames = "waypoint_names";
std::string_view constexpr kWaypointCallbacks = "waypoint_callbacks";
std::string_view constexpr kApi = "api";
std::string_view constexpr kMode = "mode";
std::string_view constexpr kTravelMode = "travelmode";
std::string_view constexpr kDirAction = "dir_action";
std::string_view constexpr kOptimize = "optimize";
std::string_view constexpr kRef = "ref";
std::string_view constexpr kRefName = "ref_name";
std::string_view constexpr kCallback = "callback";
std::string_view constexpr kCallbackLabel = "callback_label";
std::string_view constexpr kAvoid = "avoid";
}  // namespace route_v2

namespace search
{
std::string_view constexpr kQuery = "query";
std::string_view constexpr kLocale = "locale";
std::string_view constexpr kSearchOnMap = "map";
}  // namespace search

namespace highlight_feature
{
std::string_view constexpr kHighlight = "highlight";
}  // namespace highlight_feature

// See also kGe0Prefixes in ge0/parser.hpp
constexpr std::array<std::string_view, 3> kLegacyMwmPrefixes = {{"mapsme://", "mwm://", "mapswithme://"}};

bool ParseBool(std::string const & value)
{
  std::string const lowerValue = strings::MakeLowerCase(value);
  return lowerValue == "1" || lowerValue == "true" || lowerValue == "yes";
}

bool ParseLatLon(std::string const & key, std::string const & value, double & lat, double & lon)
{
  size_t const firstComma = value.find(',');
  if (firstComma == std::string::npos)
  {
    LOG(LWARNING, ("Map API: no comma between lat and lon for key:", key, " value:", value));
    return false;
  }

  if (!strings::to_double(value.substr(0, firstComma), lat) || !strings::to_double(value.substr(firstComma + 1), lon))
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

bool IsRouteCurrentLocation(std::string const & value)
{
  return value == route_v2::kOriginCurrentLocation || value == route_v2::kOriginCurrentLocationKebab;
}

bool ParseRoutePoint(std::string const & key, std::string const & value, RoutePoint & point)
{
  double lat = 0.0;
  double lon = 0.0;
  if (!ParseLatLon(key, value, lat, lon))
    return false;

  point.m_org = mercator::FromLatLon(lat, lon);
  point.m_isMyPosition = false;
  return true;
}

bool ParseRouteMode(std::string const & value, std::string & routingType)
{
  using namespace route;

  std::string const lowerValue = strings::MakeLowerCase(value);
  if (lowerValue == "drive" || lowerValue == "driving" || lowerValue == "car" || lowerValue == kRouteTypeVehicle)
    routingType = kRouteTypeVehicle;
  else if (lowerValue == "walk" || lowerValue == "walking" || lowerValue == kRouteTypePedestrian)
    routingType = kRouteTypePedestrian;
  else if (lowerValue == "bike" || lowerValue == "bicycling" || lowerValue == kRouteTypeBicycle)
    routingType = kRouteTypeBicycle;
  else if (lowerValue == kRouteTypeTransit)
    routingType = kRouteTypeTransit;
  else
    return false;
  return true;
}

bool ParseOriginHeading(std::string const & value, m2::PointD & startDirection)
{
  double heading = 0.0;
  if (!strings::to_double(value, heading) || !std::isfinite(heading) || heading < 0.0 || heading > 360.0)
    return false;

  // Heading is clockwise from north; convert to a counter-clockwise-from-east unit vector.
  double const angle = math::DegToRad(90.0 - heading);
  startDirection = {std::cos(angle), std::sin(angle)};
  return true;
}

// A helper for SetUrlAndParse() below.
// kGe0Prefixes supports API and ge0 links, kLegacyMwmPrefixes - only API.
std::tuple<size_t, bool> FindUrlPrefix(std::string const & url)
{
  for (auto const prefix : ge0::Ge0Parser::kGe0Prefixes)
    if (url.starts_with(prefix))
      return {prefix.size(), true};
  for (auto const prefix : kLegacyMwmPrefixes)
    if (url.starts_with(prefix))
      return {prefix.size(), false};
  return {std::string::npos, false};
}
}  // namespace

ParsedMapApi::UrlType ParsedMapApi::SetUrlAndParse(std::string const & raw)
{
  Reset();
  SCOPE_GUARD(guard, [this]
  {
    if (m_requestType == UrlType::Incorrect)
      Reset();
  });

  if (auto const [prefix, checkForGe0Link] = FindUrlPrefix(raw); prefix != std::string::npos)
  {
    std::string const normalizedUrl = "om://" + raw.substr(prefix);
    url::Url const url{normalizedUrl};
    if (!url.IsValid())
      return m_requestType = UrlType::Incorrect;

    std::string const & type = url.GetHost();
    // The URL is prefixed by one of the kGe0Prefixes or kLegacyMwmPrefixes prefixes
    // => check for well-known API methods first.
    if (type == "v2" && (url.GetPath() == route_v2::kPathDir || url.GetPath() == route_v2::kPathNav))
    {
      // Fragments are not route parameters; the general URL parser also supports fragment parameters.
      url::Url const queryUrl{normalizedUrl.substr(0, normalizedUrl.find('#'))};
      return m_requestType = ParseRouteV2(queryUrl) ? UrlType::Route : UrlType::Incorrect;
    }
    else if (type == "map")
    {
      bool correctOrder = true;
      url.ForEachParam([&correctOrder, this](auto const & key, auto const & value)
      { ParseMapParam(key, value, correctOrder); });

      if (m_mapPoints.empty() || !correctOrder)
        return m_requestType = UrlType::Incorrect;

      return m_requestType = UrlType::Map;
    }
    else if (type == "route")
    {
      size_t paramIndex = 0;
      url.ForEachParam([this, &paramIndex](auto const & key, auto const & value)
      { ParseRouteParam(key, value, paramIndex); });
      return m_requestType = paramIndex == 5 ? UrlType::Route : UrlType::Incorrect;
    }
    else if (type == "search")
    {
      url.ForEachParam([this](auto const & key, auto const & value) { ParseSearchParam(key, value); });
      if (m_searchRequest.m_query.empty())
        return m_requestType = UrlType::Incorrect;

      return m_requestType = UrlType::Search;
    }
    else if (type == "crosshair")
    {
      url.ForEachParam([this](auto const & key, auto const & value) { ParseCommonParam(key, value); });

      return m_requestType = UrlType::Crosshair;
    }
    else if (type == "menu")
    {
      url.ForEachParam([this](auto const & key, auto const & value) { ParseInAppFeatureHighlightParam(key, value); });
      return m_requestType = UrlType::Menu;
    }
    else if (type == "settings")
    {
      url.ForEachParam([this](auto const & key, auto const & value) { ParseInAppFeatureHighlightParam(key, value); });
      return m_requestType = UrlType::Settings;
    }
    else if (type == "oauth2")
    {
      if (url.GetPath() != "osm/callback")
        return m_requestType = UrlType::Incorrect;

      url.ForEachParam([this](auto const & key, auto const & value)
      {
        if (key == "code")
          m_oauth2code = value;
      });

      if (m_oauth2code.empty())
        return m_requestType = UrlType::Incorrect;
      else
        return m_requestType = UrlType::OAuth2;
    }
    else if (checkForGe0Link)
    {
      // The URL is prefixed by one of the kGe0Prefixes AND doesn't match any supported API call.
      ge0::Ge0Parser::Result info;

      // First try a ge0 short link (base64-encoded zoom + coordinates).
      if (ge0::Ge0Parser{}.ParseAfterPrefix(raw, prefix, info))
      {
        m_zoomLevel = info.m_zoomLevel;
        m_mapPoints.push_back({info.m_lat, info.m_lon, info.m_name, "" /* m_id */, "" /* m_style */});
        return m_requestType = UrlType::Map;
      }

      // Then try clear decimal coordinates: omaps.app/<lat>,<lon>[/<name>]?z=<zoom>.
      // Strip the query and fragment first, mirroring url-processor which matches only the pathname.
      std::string_view path{raw};
      path.remove_prefix(prefix);
      if (auto const q = path.find_first_of("?#"); q != std::string_view::npos)
        path = path.substr(0, q);

      if (ge0::Ge0Parser::ParseClearCoordinates(path, info))
      {
        // Zoom comes from "?z=" only, see kDefaultClearCoordinatesZoom.
        m_zoomLevel = kDefaultClearCoordinatesZoom;
        if (auto const * z = url.GetParamValue(std::string{map::kZoomLevel}))
        {
          uint32_t parsedZoom;
          if (strings::to_uint(*z, parsedZoom) && parsedZoom >= 1 && parsedZoom <= kMaxClearCoordinatesZoom)
            m_zoomLevel = parsedZoom;
        }
        m_mapPoints.push_back({info.m_lat, info.m_lon, info.m_name, "" /* m_id */, "" /* m_style */});
        return m_requestType = UrlType::Map;
      }

      return m_requestType = UrlType::Incorrect;
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
      return m_requestType = UrlType::Search;
    }
    else
    {
      // The URL doesn't have "q=" parameter => convert to a Map API request.
      ASSERT(info.IsLatLonValid(), ());
      m_centerLatLon = {info.m_lat, info.m_lon};
      m_zoomLevel = info.m_zoom;
      m_mapPoints.push_back({info.m_lat, info.m_lon, info.m_label, "" /* m_id */, "" /* m_style */});
      return m_requestType = UrlType::Map;
    }
  }
  UNREACHABLE();
}

void ParsedMapApi::ParseMapParam(std::string const & key, std::string const & value, bool & correctOrder)
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
    if (value.find("://") == std::string::npos)
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

void ParsedMapApi::ParseRouteParam(std::string const & key, std::string const & value, size_t & paramIndex)
{
  using namespace route;
  if (key == kVersion)
  {
    if (!strings::to_int(value, m_version))
      m_version = 0;
    return;
  }
  ParseCommonParam(key, value);

  // Legacy links consume this sequence and ignore parameters outside it, including trailing duplicates.
  constexpr std::array<std::string_view, 5> kParams = {kSourceLatLon, kSourceName, kDestLatLon, kDestName, kRouteType};
  if (paramIndex == kParams.size() || key != kParams[paramIndex])
    return;

  if (key == kSourceLatLon || key == kDestLatLon)
  {
    RoutePoint point;
    if (!ParseRoutePoint(key, value, point))
      return;
    m_routePoints.push_back(std::move(point));
  }
  else if (key == kRouteType)
  {
    auto const type = strings::MakeLowerCase(value);
    if (type != kRouteTypeVehicle && type != kRouteTypePedestrian && type != kRouteTypeBicycle &&
        type != kRouteTypeTransit)
      return;
    m_routingType = type;
  }
  else
    m_routePoints.back().m_name = value;
  ++paramIndex;
}

bool ParsedMapApi::ParseRouteV2(url::Url const & url)
{
  using namespace route_v2;
  m_version = 2;
  std::string origin = std::string(kOriginCurrentLocation);
  std::string destination;
  std::string mode = "drive";
  std::string heading;
  bool hasHeading = false;
  std::string dirAction;
  RoutePoint start, finish;
  std::vector<std::string> waypoints, names, callbacks;

  // Values are decoded once by Url. Split on decoded pipes, retaining empty slots for aligned metadata.
  // Collect before validating so every recognized field consistently uses its last occurrence.
  url.ForEachParam([&](auto const & key, auto const & value)
  {
    if (key == kOrigin)
      origin = value;
    else if (key == kDestination)
      destination = value;
    else if (key == kOriginName)
      start.m_name = value;
    else if (key == kDestinationName)
      finish.m_name = value;
    else if (key == kOriginCallback)
      start.m_callback = value;
    else if (key == kDestinationCallback)
      finish.m_callback = value;
    else if (key == kOriginHeading)
    {
      heading = value;
      hasHeading = true;
    }
    else if (key == kMode || key == kTravelMode)
      mode = value;
    else if (key == kWaypoints)
      strings::ParseCSVRow(value, '|', waypoints);
    else if (key == kWaypointNames)
      strings::ParseCSVRow(value, '|', names);
    else if (key == kWaypointCallbacks)
      strings::ParseCSVRow(value, '|', callbacks);
    else if (key == kDirAction)
      dirAction = value;
    else if (key == kOptimize)
      m_optimizeRoutePoints = ParseBool(value);
    else if (key == kRefName)
      m_appName = value;
    else if (key == kCallback)
      m_globalBackUrl = value;
    else if (key == kApi || key == kRef || key == kCallbackLabel || key == kAvoid)
      LOG(LWARNING, ("Route API v2 parameter is not applied:", key, value));
    else
      LOG(LWARNING, ("Unsupported Route API v2 parameter:", key, value));
  });

  if (!dirAction.empty() && dirAction != "navigate")
    LOG(LWARNING, ("Unsupported route action:", dirAction));
  m_startRouteNavigation = url.GetPath() == kPathNav || dirAction == "navigate";
  if (!ParseRouteMode(mode, m_routingType) || !ParseRoutePoint(std::string(kDestination), destination, finish))
    return false;
  if (hasHeading && !ParseOriginHeading(heading, m_startDirection))
    return false;

  // Navigation starts at the user's position; explicit origins only affect previews.
  if (m_startRouteNavigation || IsRouteCurrentLocation(origin))
    start.m_isMyPosition = true;
  else if (!ParseRoutePoint(std::string(kOrigin), origin, start))
    return false;
  m_routePoints.push_back(std::move(start));
  for (size_t i = 0; i < waypoints.size(); ++i)
  {
    if (waypoints[i].empty())
      continue;
    if (m_routePoints.size() > RoutePointsLayout::kMaxIntermediatePointsCount)
      return false;
    RoutePoint point;
    if (!ParseRoutePoint(std::string(kWaypoints), waypoints[i], point))
      return false;
    if (i < names.size())
      point.m_name = std::move(names[i]);
    if (i < callbacks.size())
      point.m_callback = std::move(callbacks[i]);
    m_routePoints.push_back(std::move(point));
  }
  m_routePoints.push_back(std::move(finish));
  return true;
}

void ParsedMapApi::ParseSearchParam(std::string const & key, std::string const & value)
{
  using namespace search;

  if (key == kQuery)
    m_searchRequest.m_query = value;
  else if (key == kLocale)
    m_searchRequest.m_locale = value;
  else if (key == kSearchOnMap)
    m_searchRequest.m_isSearchOnMap = true;
  else
    ParseCommonParam(key, value);
}

void ParsedMapApi::ParseCommonParam(std::string const & key, std::string const & value)
{
  if (key == kAppName)
  {
    m_appName = value;
  }
  else if (key == kCenterLatLon)
  {
    if (double lat, lon; ParseLatLon(key, value, lat, lon))
      m_centerLatLon = {lat, lon};
    else
      LOG(LWARNING, ("Incorrect 'cll':", value));
  }
}

InAppFeatureHighlightRequest::InAppFeatureType ParseInAppFeatureType(std::string const & value)
{
  if (value == "track-recorder")
    return InAppFeatureHighlightRequest::InAppFeatureType::TrackRecorder;
  if (value == "icloud")
    return InAppFeatureHighlightRequest::InAppFeatureType::iCloud;
  LOG(LERROR, ("Incorrect InAppFeatureType:", value));
  return InAppFeatureHighlightRequest::InAppFeatureType::None;
}

void ParsedMapApi::ParseInAppFeatureHighlightParam(std::string const & key, std::string const & value)
{
  if (key == highlight_feature::kHighlight)
    m_inAppFeatureHighlightRequest.m_feature = ParseInAppFeatureType(value);
}

void ParsedMapApi::Reset()
{
  m_requestType = UrlType::Incorrect;
  m_mapPoints.clear();
  m_routePoints.clear();
  m_searchRequest = {};
  m_oauth2code = {};
  m_globalBackUrl = {};
  m_appName = {};
  m_centerLatLon = ms::LatLon::Invalid();
  m_routingType = {};
  m_startDirection = m2::PointD::Zero();
  m_optimizeRoutePoints = false;
  m_startRouteNavigation = false;
  m_version = 0;
  m_zoomLevel = 0.0;
  m_goBackOnBalloonClick = false;
  m_inAppFeatureHighlightRequest = {};
}

void ParsedMapApi::ExecuteMapApiRequest(Framework & fm) const
{
  ASSERT_EQUAL(m_requestType, UrlType::Map, ("Must be a Map API request"));
  VERIFY(m_mapPoints.size() > 0, ("Map API request must have at least one point"));

  // Clear every current API-mark.
  auto editSession = fm.GetBookmarkManager().GetEditSession();
  editSession.ClearGroup(UserMark::Type::API);
  editSession.SetIsVisible(UserMark::Type::API, true);

  // Add marks from the request.
  m2::RectD viewport;
  for (auto const & [lat, lon, name, id, style] : m_mapPoints)
  {
    m2::PointD const glPoint(mercator::FromLatLon(lat, lon));
    auto * mark = editSession.CreateUserMark<ApiMarkPoint>(glPoint);
    mark->SetName(name);
    mark->SetApiID(id);
    mark->SetStyle(style::GetSupportedStyle(style));
    viewport.Add(glPoint);
  }

  // Calculate the optimal viewport.
  VERIFY(viewport.IsValid(), ());
  m2::PointD const center = viewport.Center();

  // Calculate the best zoom.
  int zoomLevel;
  if (m_mapPoints.size() == 1)
    if (m_zoomLevel >= 1.0)  // 0 means uninitialized/not passed to the API.
      zoomLevel = std::min(scales::GetUpperComfortScale(), static_cast<int>(m_zoomLevel));
    else
      zoomLevel = scales::GetUpperComfortScale();
  else
    zoomLevel = df::GetDrawTileScale(viewport);

  // Always hide current map selection.
  fm.DeactivateMapSelection();

  // Set viewport and stop follow mode.
  fm.StopLocationFollow();

  // ShowRect function interferes with ActivateMapSelection and we have strange behaviour as a result.
  // Use more obvious SetModelViewCenter here.
  /// @todo Funny, but animation is still present, but now centering works fine.
  /// Looks like there is one more set viewport call somewhere.
  fm.SetViewportCenter(center, zoomLevel, false /* isAnim */, true /* trackVisibleViewport */);

  // Don't show the place page in case of multiple points.
  if (m_mapPoints.size() > 1)
    return;

  place_page::BuildInfo info;
  info.m_needAnimationOnSelection = false;
  info.m_mercator = mercator::FromLatLon(m_mapPoints[0].m_lat, m_mapPoints[0].m_lon);
  // Other details will be filled in by BuildPlacePageInfo().
  fm.BuildAndSetPlacePageInfo(info);
}

void ParsedMapApi::ExecuteRouteApiRequest(Framework & fm) const
{
  CHECK_EQUAL(m_requestType, UrlType::Route, ("Must be a Route API request"));
  CHECK_GREATER_OR_EQUAL(m_routePoints.size(), 2, ());
  std::vector<RouteMarkData> points;
  points.reserve(m_routePoints.size());
  for (auto const & point : m_routePoints)
  {
    RouteMarkData data;
    data.m_title = point.m_name;
    data.m_isMyPosition = point.m_isMyPosition;
    data.m_position = point.m_org;
    data.m_callback = point.m_callback;
    points.push_back(std::move(data));
  }
  // origin_callback is reserved, not an executable stop callback.
  points.front().m_callback.clear();
  auto & rm = fm.GetRoutingManager();
  rm.SetRouter(routing::FromString(m_routingType));
  rm.ReplaceRoutePoints(std::move(points), m_optimizeRoutePoints);
  rm.BuildRoute(routing::RouterDelegate::kNoTimeout, m_startDirection);
}

std::string DebugPrint(ParsedMapApi::UrlType type)
{
  switch (type)
  {
  case ParsedMapApi::UrlType::Incorrect: return "Incorrect";
  case ParsedMapApi::UrlType::Map: return "Map";
  case ParsedMapApi::UrlType::Route: return "Route";
  case ParsedMapApi::UrlType::Search: return "Search";
  case ParsedMapApi::UrlType::Crosshair: return "Crosshair";
  case ParsedMapApi::UrlType::OAuth2: return "OAuth2";
  case ParsedMapApi::UrlType::Menu: return "Menu";
  case ParsedMapApi::UrlType::Settings: return "Settings";
  }
  UNREACHABLE();
}
}  // namespace url_scheme
