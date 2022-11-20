#include "map/mwm_url.hpp"

#include "map/api_mark_point.hpp"
#include "map/bookmark_manager.hpp"

#include "geometry/mercator.hpp"
#include "indexer/scales.hpp"

#include "drape_frontend/visual_params.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <array>

namespace url_scheme
{
using namespace std;

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

namespace
{
std::array<std::string, 4> const kAvailableSchemes = {{"mapswithme", "mwm", "mapsme", "om"}};

ParsedMapApi::UrlType GetUrlType(url::Url const & url)
{
  if (!base::IsExist(kAvailableSchemes, url.GetScheme()))
    return ParsedMapApi::UrlType::Incorrect;

  auto const & type = url.GetHost();
  if (type == "map")
    return ParsedMapApi::UrlType::Map;
  if (type == "route")
    return ParsedMapApi::UrlType::Route;
  if (type == "search")
    return ParsedMapApi::UrlType::Search;
  if (type == "crosshair")
    return ParsedMapApi::UrlType::Crosshair;

  return ParsedMapApi::UrlType::Incorrect;
}

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
      url.ForEachParam([&points, &correctOrder, this](auto const & key, auto const & value)
      {
        ParseMapParam(key, value, points, correctOrder);
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
      url.ForEachParam([&pattern, this](auto const & key, auto const & value)
      {
        ParseRouteParam(key, value, pattern);
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
      url.ForEachParam([this](auto const & key, auto const & value)
      {
        ParseSearchParam(key, value);
      });
      if (m_searchRequest.m_query.empty())
      {
        m_searchRequest = SearchRequest();
        return false;
      }

      return true;
    }
    case UrlType::Crosshair:
    {
      url.ForEachParam([this](auto const & key, auto const & value)
      {
         ParseCommonParam(key, value);
      });
      return true;
    }
  }
  UNREACHABLE();
}

void ParsedMapApi::ParseMapParam(std::string const & key, std::string const & value,
                                 vector<ApiPoint> & points, bool & correctOrder)
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
    double lat = 0.0;
    double lon = 0.0;
    if (!ParseLatLon(key, value, lat, lon))
    {
      LOG(LWARNING, ("Incorrect 'cll':", value));
      return;
    }
    m_centerLatLon = ms::LatLon(lat, lon);
  }
}

void ParsedMapApi::Reset()
{
  m_routePoints = {};
  m_searchRequest = {};
  m_globalBackUrl ={};
  m_appName = {};
  m_centerLatLon = {};
  m_routingType = {};
  m_version = 0;
  m_zoomLevel = 0.0;
  m_goBackOnBalloonClick = false;
  m_isValid = false;
}

bool ParsedMapApi::GetViewportParams(m2::PointD & center, double & scale) const
{
  auto const & markIds = m_bmManager->GetUserMarkIds(UserMark::Type::API);
  if (markIds.size() == 1 && m_zoomLevel >= 1)
  {
    scale = min(static_cast<double>(scales::GetUpperComfortScale()), m_zoomLevel);
    center = m_bmManager->GetUserMark(*markIds.begin())->GetPivot();
    return true;
  }
  else
  {
    m2::RectD result;
    for (auto markId : markIds)
      result.Add(m_bmManager->GetUserMark(markId)->GetPivot());

    if (result.IsValid())
    {
      center = result.Center();
      scale = df::GetDrawTileScale(result);
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
  case ParsedMapApi::UrlType::Crosshair: return "Crosshair";
  }
  UNREACHABLE();
}
}  // namespace url_scheme
