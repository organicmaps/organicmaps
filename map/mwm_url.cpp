#include "mwm_url.hpp"

#include "map/api_mark_point.hpp"
#include "map/bookmark_manager.hpp"

#include "geometry/mercator.hpp"
#include "indexer/scales.hpp"

#include "drape_frontend/visual_params.hpp"

#include "coding/uri.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "std/algorithm.hpp"
#include "std/bind.hpp"


namespace url_scheme
{

namespace
{
string const kLatLon = "ll";
string const kSourceLatLon = "sll";
string const kDestLatLon = "dll";
string const kZoomLevel = "z";
string const kName = "n";
string const kSourceName = "saddr";
string const kDestName = "daddr";
string const kId = "id";
string const kStyle = "s";
string const kBackUrl = "backurl";
string const kVersion = "v";
string const kAppName = "appname";
string const kBalloonAction = "balloonaction";
string const kRouteType = "type";

static int const INVALID_LAT_VALUE = -1000;

bool ParseLatLon(double & lat, double & lon, string const & key, string const & value)
{
  size_t const firstComma = value.find(',');
  if (firstComma == string::npos)
  {
    LOG(LWARNING, ("Map API: no comma between lat and lon for 'll' key", key, value));
    return false;
  }

  if (value.find(',', firstComma + 1) != string::npos)
  {
    LOG(LWARNING, ("Map API: more than one comma in a value for 'll' key", key, value));
    return false;
  }

  if (!strings::to_double(value.substr(0, firstComma), lat) ||
      !strings::to_double(value.substr(firstComma + 1), lon))
  {
    LOG(LWARNING, ("Map API: can't parse lat,lon for 'll' key", key, value));
    return false;
  }

  if (!MercatorBounds::ValidLat(lat) || !MercatorBounds::ValidLon(lon))
  {
    LOG(LWARNING, ("Map API: incorrect value for lat and/or lon", key, value, lat, lon));
    return false;
  }
  return true;
}

bool IsInvalidApiPoint(ApiPoint const & p) { return p.m_lat == INVALID_LAT_VALUE; }

}  // unnames namespace

ParsedMapApi::ParsedMapApi()
  : m_bmManager(nullptr)
  , m_version(0)
  , m_zoomLevel(0.0)
  , m_goBackOnBalloonClick(false)
{
}

void ParsedMapApi::SetBookmarkManager(BookmarkManager * manager)
{
  m_bmManager = manager;
}
ParsingResult ParsedMapApi::SetUriAndParse(string const & url)
{
  Reset();
  ParsingResult const res = Parse(url_scheme::Uri(url));
  m_isValid = res != ParsingResult::Incorrect;
  return res;
}

ParsingResult ParsedMapApi::Parse(Uri const & uri)
{
  string const & scheme = uri.GetScheme();
  string const & path = uri.GetPath();
  bool const isRoutePath = path == "route";
  if ((scheme != "mapswithme" && scheme != "mwm" && scheme != "mapsme") ||
      (path != "map" && !isRoutePath))
    return ParsingResult::Incorrect;

  if (isRoutePath)
  {
    vector<string> pattern{kSourceLatLon, kSourceName, kDestLatLon, kDestName, kRouteType};
    if (!uri.ForEachKeyValue(bind(&ParsedMapApi::RouteKeyValue, this, _1, _2, ref(pattern))))
      return ParsingResult::Incorrect;

    if (pattern.size() != 0)
      return ParsingResult::Incorrect;

    if (m_routePoints.size() != 2)
    {
      ASSERT(false, ());
      return ParsingResult::Incorrect;
    }

    return ParsingResult::Route;
  }
  else
  {
    ASSERT(m_bmManager != nullptr, ());
    UserMarkControllerGuard guard(*m_bmManager, UserMarkType::API_MARK);
    vector<ApiPoint> points;
    if (!uri.ForEachKeyValue(bind(&ParsedMapApi::AddKeyValue, this, _1, _2, ref(points))))
      return ParsingResult::Incorrect;

    points.erase(remove_if(points.begin(), points.end(), &IsInvalidApiPoint), points.end());
    if ((isRoutePath && (points.size() < 2 || m_routingType.empty())) || points.empty())
      return ParsingResult::Incorrect;

    for (auto const & p : points)
    {
      m2::PointD glPoint(MercatorBounds::FromLatLon(p.m_lat, p.m_lon));
      ApiMarkPoint * mark = static_cast<ApiMarkPoint *>(guard.m_controller.CreateUserMark(glPoint));
      mark->SetName(p.m_name);
      mark->SetID(p.m_id);
      mark->SetStyle(style::GetSupportedStyle(p.m_style, p.m_name, ""));
    }

    return ParsingResult::Map;
  }
}

bool ParsedMapApi::RouteKeyValue(string key, string const & value, vector<string> & pattern)
{
  if (key != pattern.front())
    return false;

  if (key == kSourceLatLon || key == kDestLatLon)
  {
    double lat = 0.0;
    double lon = 0.0;
    if (!ParseLatLon(lat, lon, key, value))
      return false;

    RoutePoint p;
    p.m_org = MercatorBounds::FromLatLon(lat, lon);
    m_routePoints.push_back(p);
  }
  else if (key == kSourceName || key == kDestName)
  {
    m_routePoints.back().m_name = value;
  }
  else if (key == kRouteType)
  {
    string const lowerValue = strings::MakeLowerCase(value);
    if (lowerValue == "pedestrian" || lowerValue == "vehicle" || lowerValue == "bicycle")
    {
      m_routingType = lowerValue;
    }
    else
    {
      LOG(LWARNING, ("Incorrect routing type:", value));
      return false;
    }
  }

  pattern.erase(pattern.begin());
  return true;
}

bool ParsedMapApi::AddKeyValue(string key, string const & value, vector<ApiPoint> & points)
{
  strings::AsciiToLower(key);

  if (key == kLatLon)
  {
    points.push_back(ApiPoint());
    points.back().m_lat = INVALID_LAT_VALUE;

    double lat = 0.0;
    double lon = 0.0;
    if (!ParseLatLon(lat, lon, key, value))
      return false;

    points.back().m_lat = lat;
    points.back().m_lon = lon;
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
      return false;
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
      return false;
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
      return false;
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
  return true;
}

void ParsedMapApi::Reset()
{
  m_globalBackUrl.clear();
  m_appTitle.clear();
  m_version = 0;
  m_zoomLevel = 0.0;
  m_goBackOnBalloonClick = false;
}

bool ParsedMapApi::GetViewportRect(m2::RectD & rect) const
{
  ASSERT(m_bmManager != nullptr, ());
  UserMarkControllerGuard guard(*m_bmManager, UserMarkType::API_MARK);

  size_t markCount = guard.m_controller.GetUserMarkCount();
  if (markCount == 1 && m_zoomLevel >= 1)
  {
    double zoom = min(static_cast<double>(scales::GetUpperComfortScale()), m_zoomLevel);
    rect = df::GetRectForDrawScale(zoom, guard.m_controller.GetUserMark(0)->GetPivot());
    return true;
  }
  else
  {
    m2::RectD result;
    for (size_t i = 0; i < guard.m_controller.GetUserMarkCount(); ++i)
      result.Add(guard.m_controller.GetUserMark(i)->GetPivot());

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
  UserMarkControllerGuard guard(*m_bmManager, UserMarkType::API_MARK);

  if (guard.m_controller.GetUserMarkCount() != 1)
    return nullptr;

  return static_cast<ApiMarkPoint const *>(guard.m_controller.GetUserMark(0));
}

}
