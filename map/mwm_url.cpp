#include "mwm_url.hpp"

#include "map/api_mark_point.hpp"
#include "map/bookmark_manager.hpp"

#include "geometry/mercator.hpp"
#include "indexer/scales.hpp"

#include "drape_frontend/visual_params.hpp"

#include "platform/marketing_service.hpp"
#include "platform/settings.hpp"

#include "coding/uri.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "std/algorithm.hpp"
#include "std/bind.hpp"

#include <array>

namespace url_scheme
{
namespace lead
{
char const * kFrom = marketing::kFrom;
char const * kType = marketing::kType;
char const * kName = marketing::kName;
char const * kContent = marketing::kContent;
char const * kKeyword = marketing::kKeyword;

struct CampaignDescription
{
  void Write() const
  {
    if (!IsValid())
    {
      LOG(LERROR, ("Invalid campaign description"));
      return;
    }

    marketing::Settings::Set(kFrom, m_from);
    marketing::Settings::Set(kType, m_type);
    marketing::Settings::Set(kName, m_name);

    if (!m_content.empty())
      marketing::Settings::Set(kContent, m_content);

    if (!m_keyword.empty())
      marketing::Settings::Set(kKeyword, m_keyword);
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

namespace catalogue
{
char const * kId = "id";
char const * kName = "name";
}

namespace
{
enum class ApiURLType
{
  Incorrect,
  Map,
  Route,
  Search,
  Lead,
  Catalogue
};

std::array<std::string, 3> const kAvailableSchemes = {{"mapswithme", "mwm", "mapsme"}};

ApiURLType URLType(Uri const & uri)
{
  if (std::find(kAvailableSchemes.begin(), kAvailableSchemes.end(), uri.GetScheme()) == kAvailableSchemes.end())
    return ApiURLType::Incorrect;

  auto const path = uri.GetPath();
  if (path == "map")
    return ApiURLType::Map;
  if (path == "route")
    return ApiURLType::Route;
  if (path == "search")
    return ApiURLType::Search;
  if (path == "lead")
    return ApiURLType::Lead;
  if (path == "catalogue")
    return ApiURLType::Catalogue;

  return ApiURLType::Incorrect;
}

bool ParseLatLon(string const & key, string const & value, double & lat, double & lon)
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

  if (!MercatorBounds::ValidLat(lat) || !MercatorBounds::ValidLon(lon))
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

ParsedMapApi::ParsingResult ParsedMapApi::SetUriAndParse(string const & url)
{
  Reset();

  if (!strings::StartsWith(url, "mapswithme://") && !strings::StartsWith(url, "mwm://") &&
      !strings::StartsWith(url, "mapsme://"))
  {
    return ParsingResult::Incorrect;
  }

  ParsingResult const res = Parse(url_scheme::Uri(url));
  m_isValid = res != ParsingResult::Incorrect;
  return res;
}

ParsedMapApi::ParsingResult ParsedMapApi::Parse(Uri const & uri)
{
  switch (URLType(uri))
  {
    case ApiURLType::Incorrect:
      return ParsingResult::Incorrect;
    case ApiURLType::Map:
    {
      vector<ApiPoint> points;
      auto const result = uri.ForEachKeyValue([&points, this](string const & key, string const & value)
                                              {
                                                return AddKeyValue(key, value, points);
                                              });
      if (!result)
        return ParsingResult::Incorrect;

      if (points.empty())
        return ParsingResult::Incorrect;

      ASSERT(m_bmManager != nullptr, ());
      auto editSession = m_bmManager->GetEditSession();
      for (auto const & p : points)
      {
        m2::PointD glPoint(MercatorBounds::FromLatLon(p.m_lat, p.m_lon));
        auto * mark = editSession.CreateUserMark<ApiMarkPoint>(glPoint);
        mark->SetName(p.m_name);
        mark->SetApiID(p.m_id);
        mark->SetStyle(style::GetSupportedStyle(p.m_style));
      }

      return ParsingResult::Map;
    }
    case ApiURLType::Route:
    {
      m_routePoints.clear();
      using namespace route;
      vector<string> pattern{kSourceLatLon, kSourceName, kDestLatLon, kDestName, kRouteType};
      auto const result = uri.ForEachKeyValue([&pattern, this](string const & key, string const & value)
                                              {
                                                return RouteKeyValue(key, value, pattern);
                                              });

      if (!result)
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
    case ApiURLType::Search:
    {
      SearchRequest request;
      auto const result = uri.ForEachKeyValue([&request, this](string const & key, string const & value)
                                              {
                                                return SearchKeyValue(key, value, request);
                                              });
      if (!result)
        return ParsingResult::Incorrect;
      
      m_request = request;
      return request.m_query.empty() ? ParsingResult::Incorrect : ParsingResult::Search;
    }
    case ApiURLType::Lead:
    {
      lead::CampaignDescription description;
      auto result = uri.ForEachKeyValue([&description, this](string const & key, string const & value)
                                        {
                                          return LeadKeyValue(key, value, description);
                                        });
      if (!result)
        return ParsingResult::Incorrect;

      if (!description.IsValid())
        return ParsingResult::Incorrect;

      description.Write();
      return ParsingResult::Lead;
    }
    case ApiURLType::Catalogue:
    {
      CatalogItem item;
      auto const result = uri.ForEachKeyValue([&item, this](string const & key, string const & value)
                                              {
                                                return CatalogKeyValue(key, value, item);
                                              });

      if (!result)
        return ParsingResult::Incorrect;

      if (item.m_id.empty())
        return ParsingResult::Incorrect;

      m_catalogItem = item;
      return ParsingResult::Catalogue;
    }
  }
  UNREACHABLE();
}

bool ParsedMapApi::RouteKeyValue(string const & key, string const & value, vector<string> & pattern)
{
  using namespace route;

  if (pattern.empty() || key != pattern.front())
    return false;

  if (key == kSourceLatLon || key == kDestLatLon)
  {
    double lat = 0.0;
    double lon = 0.0;
    if (!ParseLatLon(key, value, lat, lon))
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
    if (lowerValue == kRouteTypePedestrian || lowerValue == kRouteTypeVehicle ||
        lowerValue == kRouteTypeBicycle || lowerValue == kRouteTypeTransit)
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

bool ParsedMapApi::AddKeyValue(string const & key, string const & value, vector<ApiPoint> & points)
{
  using namespace map;

  if (key == kLatLon)
  {
    double lat = 0.0;
    double lon = 0.0;
    if (!ParseLatLon(key, value, lat, lon))
      return false;

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

bool ParsedMapApi::SearchKeyValue(string const & key, string const & value, SearchRequest & request) const
{
  using namespace search;

  if (key == kQuery)
  {
    if (value.empty())
      return false;

    request.m_query = value;
  }
  else if (key == kCenterLatLon)
  {
    double lat = 0.0;
    double lon = 0.0;
    if (ParseLatLon(key, value, lat, lon))
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

  return true;
}

bool ParsedMapApi::LeadKeyValue(string const & key, string const & value, lead::CampaignDescription & description) const
{
  using namespace lead;

  if (key == kFrom)
    description.m_from = value;
  else if (key == kType)
    description.m_type = value;
  else if (key == kName)
    description.m_name = value;
  else if (key == kContent)
    description.m_content = value;
  else if (key == kKeyword)
    description.m_keyword = value;
  /*
   We have to support parsing the uri which contains unregistred parameters.
   */
  return true;
}

bool ParsedMapApi::CatalogKeyValue(string const & key, string const & value, CatalogItem & item) const
{
  using namespace catalogue;

  if (key == kName)
    item.m_name = value;
  else if (key == kId)
    item.m_id = value;

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

}
