#include "mwm_url.hpp"

#include "render/scales_processor.hpp"

#include "indexer/mercator.hpp"
#include "indexer/scales.hpp"

#include "coding/uri.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "std/algorithm.hpp"
#include "std/bind.hpp"


namespace url_scheme
{

namespace
{

static int const INVALID_LAT_VALUE = -1000;

bool IsInvalidApiPoint(ApiPoint const & p) { return p.m_lat == INVALID_LAT_VALUE; }

}  // unnames namespace

ParsedMapApi::ParsedMapApi()
  : m_controller(NULL)
  , m_version(0)
  , m_zoomLevel(0.0)
  , m_goBackOnBalloonClick(false)
{
}

void ParsedMapApi::SetController(UserMarkContainer::Controller * controller)
{
  m_controller = controller;
}

bool ParsedMapApi::SetUriAndParse(string const & url)
{
  Reset();
  return Parse(url_scheme::Uri(url));
}

bool ParsedMapApi::IsValid() const
{
  ASSERT(m_controller != NULL,  ());
  return m_controller->GetUserMarkCount() > 0;
}

bool ParsedMapApi::Parse(Uri const & uri)
{
  ASSERT(m_controller != NULL, ());

  string const & scheme = uri.GetScheme();
  if ((scheme != "mapswithme" && scheme != "mwm") || uri.GetPath() != "map")
    return false;

  vector<ApiPoint> points;
  uri.ForEachKeyValue(bind(&ParsedMapApi::AddKeyValue, this, _1, _2, ref(points)));
  points.erase(remove_if(points.begin(), points.end(), &IsInvalidApiPoint), points.end());

  for (size_t i = 0; i < points.size(); ++i)
  {
    ApiPoint const & p = points[i];
    m2::PointD glPoint(MercatorBounds::FromLatLon(p.m_lat, p.m_lon));
    ApiMarkPoint * mark = static_cast<ApiMarkPoint *>(m_controller->CreateUserMark(glPoint));
    mark->SetName(p.m_name);
    mark->SetID(p.m_id);
  }

  return true;
}

void ParsedMapApi::AddKeyValue(string key, string const & value, vector<ApiPoint> & points)
{
  strings::AsciiToLower(key);

  if (key == "ll")
  {
    points.push_back(ApiPoint());
    points.back().m_lat = INVALID_LAT_VALUE;

    size_t const firstComma = value.find(',');
    if (firstComma == string::npos)
    {
      LOG(LWARNING, ("Map API: no comma between lat and lon for 'll' key", key, value));
      return;
    }

    if (value.find(',', firstComma + 1) != string::npos)
    {
      LOG(LWARNING, ("Map API: more than one comma in a value for 'll' key", key, value));
      return;
    }

    double lat, lon;
    if (!strings::to_double(value.substr(0, firstComma), lat) ||
        !strings::to_double(value.substr(firstComma + 1), lon))
    {
      LOG(LWARNING, ("Map API: can't parse lat,lon for 'll' key", key, value));
      return;
    }

    if (!MercatorBounds::ValidLat(lat) || !MercatorBounds::ValidLon(lon))
    {
      LOG(LWARNING, ("Map API: incorrect value for lat and/or lon", key, value, lat, lon));
      return;
    }

    points.back().m_lat = lat;
    points.back().m_lon = lon;
  }
  else if (key == "z")
  {
    if (!strings::to_double(value, m_zoomLevel))
      m_zoomLevel = 0.0;
  }
  else if (key == "n")
  {
    if (!points.empty())
      points.back().m_name = value;
    else
      LOG(LWARNING, ("Map API: Point name with no point. 'll' should come first!"));
  }
  else if (key == "id")
  {
    if (!points.empty())
      points.back().m_id = value;
    else
      LOG(LWARNING, ("Map API: Point url with no point. 'll' should come first!"));
  }
  else if (key == "backurl")
  {
    // Fix missing :// in back url, it's important for iOS
    if (value.find("://") == string::npos)
      m_globalBackUrl = value + "://";
    else
      m_globalBackUrl = value;
  }
  else if (key == "v")
  {
    if (!strings::to_int(value, m_version))
      m_version = 0;
  }
  else if (key == "appname")
  {
    m_appTitle = value;
  }
  else if (key == "balloonaction")
  {
    m_goBackOnBalloonClick = true;
  }
}

void ParsedMapApi::Reset()
{
  m_globalBackUrl.clear();
  m_appTitle.clear();
  m_version = 0;
  m_zoomLevel = 0.0;
  m_goBackOnBalloonClick = false;
}

bool ParsedMapApi::GetViewportRect(ScalesProcessor const & scales, m2::RectD & rect) const
{
  ASSERT(m_controller != NULL, ());
  size_t markCount = m_controller->GetUserMarkCount();
  if (markCount == 1 && m_zoomLevel >= 1)
  {
    double zoom = min(static_cast<double>(scales::GetUpperComfortScale()), m_zoomLevel);
    rect = scales.GetRectForDrawScale(zoom, m_controller->GetUserMark(0)->GetOrg());
    return true;
  }
  else
  {
    m2::RectD result;
    for (size_t i = 0; i < m_controller->GetUserMarkCount(); ++i)
      result.Add(m_controller->GetUserMark(i)->GetOrg());

    if (result.IsValid())
    {
      rect = result;
      return true;
    }

    return false;
  }
}

UserMark const * ParsedMapApi::GetSinglePoint() const
{
  ASSERT(m_controller != NULL, ());
  if (m_controller->GetUserMarkCount() != 1)
    return 0;

  return m_controller->GetUserMark(0);
}

}
