#include "map/mwm_url.hpp"

#include "map/api_mark_point.hpp"
#include "map/bookmark_manager.hpp"

#include "geometry/mercator.hpp"
#include "indexer/scales.hpp"

#include "drape_frontend/visual_params.hpp"

#include "platform/settings.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <array>

using namespace std;

namespace url_scheme
{
namespace lead
{
struct CampaignDescription
{
  void Write() const
  {
    if (!IsValid())
    {
      LOG(LERROR, ("Invalid campaign description"));
      return;
    }
  }

  bool IsValid() const { return !m_from.empty() && !m_type.empty() && !m_name.empty(); }
  
  string m_from;
  string m_type;
  string m_name;
  string m_content;
  string m_keyword;
};
}  // namespace lead

namespace map
{
char const * kLatLon = "ll";
char const * kZoomLevel = "z";
char const * kName = "n";
char const * kId = "id";
char const * kStyle = "s";
char const * kBackUrl = "backurl";
char const * kVersion = "v";
char const * kAppName = "appname";
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
char const * kCenterLatLon = "cll";
char const * kLocale = "locale";
char const * kSearchOnMap = "map";
}  // namespace search

namespace
{
std::array<std::string, 4> const kAvailableSchemes = {{"mapswithme", "mwm", "mapsme", "om"}};

ParsedMapApi::UrlType GetUrlType(url::Url const & url)
{
  if (std::find(kAvailableSchemes.begin(), kAvailableSchemes.end(), url.GetScheme()) == kAvailableSchemes.end())
    return ParsedMapApi::UrlType::Incorrect;

  auto const path = url.GetPath();
  if (path == "map")
    return ParsedMapApi::UrlType::Map;
  if (path == "route")
    return ParsedMapApi::UrlType::Route;
  if (path == "search")
    return ParsedMapApi::UrlType::Search;

  return ParsedMapApi::UrlType::Incorrect;
}

bool ParseLatLon(url::Param const & param, double & lat, double & lon)
{
  string const & key = param.m_name;
  string const & value = param.m_value;

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
}  // namespace

void ParsedMapApi::SetBookmarkManager(BookmarkManager * manager)
{
  m_bmManager = manager;
}

ParsedMapApi::ParsingResult ParsedMapApi::SetUrlAndParse(string const & url)
{
  Reset();

  auto const u = url::Url(url);
  auto const urlType = GetUrlType(u);
  m_isValid = Parse(u, urlType);

  return {urlType, m_isValid};
}

bool ParsedMapApi::Parse(url::Url const & url, UrlType type)
{
  switch (type)
  {
    case UrlType::Incorrect:
      return false;
    case UrlType::Map:
    {
      vector<ApiPoint> points;
      bool correctOrder = true;
      url.ForEachParam([&points, &correctOrder, this](url::Param const & param) {
        ParseMapParam(param, points, correctOrder);
      });

      if (points.empty() || !correctOrder)
        return false;

      ASSERT(m_bmManager != nullptr, ());
      auto editSession = m_bmManager->GetEditSession();
      for (auto const & p : points)
      {
        m2::PointD glPoint(mercator::FromLatLon(p.m_lat, p.m_lon));
        auto * mark = editSession.CreateUserMark<ApiMarkPoint>(glPoint);
        mark->SetName(p.m_name);
        mark->SetApiID(p.m_id);
        mark->SetStyle(style::GetSupportedStyle(p.m_style));
      }

      return true;
    }
    case UrlType::Route:
    {
      m_routePoints.clear();
      using namespace route;
      vector<string> pattern{kSourceLatLon, kSourceName, kDestLatLon, kDestName, kRouteType};
      url.ForEachParam([&pattern, this](url::Param const & param) {
        ParseRouteParam(param, pattern);
      });

      if (pattern.size() != 0)
        return false;

      if (m_routePoints.size() != 2)
      {
        ASSERT(false, ());
        return false;
      }

      return true;
    }
    case UrlType::Search:
    {
      SearchRequest request;
      url.ForEachParam([&request, this](url::Param const & param) {
        ParseSearchParam(param, request);
      });
      if (request.m_query.empty())
        return false;
      
      m_request = request;
      return true;
    }
  }
  UNREACHABLE();
}

void ParsedMapApi::ParseMapParam(url::Param const & param, vector<ApiPoint> & points, bool & correctOrder)
{
  using namespace map;

  string const & key = param.m_name;
  string const & value = param.m_value;

  if (key == kLatLon)
  {
    double lat = 0.0;
    double lon = 0.0;
    if (!ParseLatLon(param, lat, lon))
      return;

    ApiPoint pt{.m_lat = lat, .m_lon = lon};
    points.push_back(pt);
  }
  else if (key == kZoomLevel)
  {
    if (!strings::to_double(value, m_zoomLevel))
      m_zoomLevel = 0.0;
  }
  else if (key == kName)
  {
    if (!points.empty())
    {
      points.back().m_name = value;
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
    if (!points.empty())
    {
      points.back().m_id = value;
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
    if (!points.empty())
    {
      points.back().m_style = value;
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
  else if (key == kAppName)
  {
    m_appTitle = value;
  }
  else if (key == kBalloonAction)
  {
    m_goBackOnBalloonClick = true;
  }
}

void ParsedMapApi::ParseRouteParam(url::Param const & param, vector<string> & pattern)
{
  using namespace route;

  string const & key = param.m_name;
  string const & value = param.m_value;

  if (pattern.empty() || key != pattern.front())
    return;

  if (key == kSourceLatLon || key == kDestLatLon)
  {
    double lat = 0.0;
    double lon = 0.0;
    if (!ParseLatLon(param, lat, lon))
      return;

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

void ParsedMapApi::ParseSearchParam(url::Param const & param, SearchRequest & request) const
{
  using namespace search;

  string const & key = param.m_name;
  string const & value = param.m_value;

  if (key == kQuery)
  {
    request.m_query = value;
  }
  else if (key == kCenterLatLon)
  {
    double lat = 0.0;
    double lon = 0.0;
    if (ParseLatLon(param, lat, lon))
    {
      request.m_centerLat = lat;
      request.m_centerLon = lon;
    }
  }
  else if (key == kLocale)
  {
    request.m_locale = value;
  }
  else if (key == kSearchOnMap)
  {
    request.m_isSearchOnMap = true;
  }
}

void ParsedMapApi::Reset()
{
  m_routePoints = {};
  m_request = {};
  m_globalBackUrl ={};
  m_appTitle = {};
  m_routingType = {};
  m_version = 0;
  m_zoomLevel = 0.0;
  m_goBackOnBalloonClick = false;
  m_isValid = false;
}

bool ParsedMapApi::GetViewportRect(m2::RectD & rect) const
{
  ASSERT(m_bmManager != nullptr, ());
  auto const & markIds = m_bmManager->GetUserMarkIds(UserMark::Type::API);
  if (markIds.size() == 1 && m_zoomLevel >= 1)
  {
    double zoom = min(static_cast<double>(scales::GetUpperComfortScale()), m_zoomLevel);
    rect = df::GetRectForDrawScale(zoom, m_bmManager->GetUserMark(*markIds.begin())->GetPivot());
    return true;
  }
  else
  {
    m2::RectD result;
    for (auto markId : markIds)
      result.Add(m_bmManager->GetUserMark(markId)->GetPivot());

    if (result.IsValid())
    {
      rect = result;
      return true;
    }

    return false;
  }
}

ApiMarkPoint const * ParsedMapApi::GetSinglePoint() const
{
  ASSERT(m_bmManager != nullptr, ());
  auto const & markIds = m_bmManager->GetUserMarkIds(UserMark::Type::API);
  if (markIds.size() != 1)
    return nullptr;

  return static_cast<ApiMarkPoint const *>(m_bmManager->GetUserMark(*markIds.begin()));
}

std::string DebugPrint(ParsedMapApi::UrlType type)
{
  switch(type)
  {
  case ParsedMapApi::UrlType::Incorrect: return "Incorrect";
  case ParsedMapApi::UrlType::Map: return "Map";
  case ParsedMapApi::UrlType::Route: return "Route";
  case ParsedMapApi::UrlType::Search: return "Search";
  }
  UNREACHABLE();
}
}  // namespace url_scheme
