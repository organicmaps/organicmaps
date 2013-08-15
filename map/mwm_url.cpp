#include "mwm_url.hpp"

#include "../indexer/mercator.hpp"
#include "../indexer/scales.hpp"

#include "../coding/uri.hpp"

#include "../base/logging.hpp"
#include "../base/string_utils.hpp"

#include "../std/algorithm.hpp"
#include "../std/bind.hpp"


using namespace url_scheme;

namespace
{

static int const INVALID_LAT_VALUE = -1000;

bool IsInvalidApiPoint(ApiPoint const & p) { return p.m_lat == INVALID_LAT_VALUE; }

}  // unnames namespace

ParsedMapApi::ParsedMapApi() : m_version(0), m_zoomLevel(0.0)
{
}

ParsedMapApi::ParsedMapApi(Uri const & uri) : m_version(0), m_zoomLevel(0.0)
{
  if (!Parse(uri))
    Reset();
}

bool ParsedMapApi::SetUriAndParse(string const & url)
{
  Reset();
  return Parse(url_scheme::Uri(url));
}

bool ParsedMapApi::IsValid() const
{
  return !m_points.empty();
}

bool ParsedMapApi::Parse(Uri const & uri)
{
  string const & scheme = uri.GetScheme();
  if ((scheme != "mapswithme" && scheme != "mwm") || uri.GetPath() != "map")
    return false;

  uri.ForEachKeyValue(bind(&ParsedMapApi::AddKeyValue, this, _1, _2));
  m_points.erase(remove_if(m_points.begin(), m_points.end(), &IsInvalidApiPoint), m_points.end());

  return true;
}

void ParsedMapApi::AddKeyValue(string key, string const & value)
{
  strings::AsciiToLower(key);

  if (key == "ll")
  {
    m_points.push_back(ApiPoint());
    m_points.back().m_lat = INVALID_LAT_VALUE;

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

    m_points.back().m_lat = lat;
    m_points.back().m_lon = lon;
    m_showRect = m2::Add(m_showRect, m2::PointD(lon, lat));
  }
  else if (key == "z")
  {
    if (!strings::to_double(value, m_zoomLevel))
      m_zoomLevel = 0.0;
  }
  else if (key == "n")
  {
    if (!m_points.empty())
      m_points.back().m_name = value;
    else
      LOG(LWARNING, ("Map API: Point name with no point. 'll' should come first!"));
  }
  else if (key == "id")
  {
    if (!m_points.empty())
      m_points.back().m_id = value;
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
}

void ParsedMapApi::Reset()
{
  m_points.clear();
  m_globalBackUrl.clear();
  m_appTitle.clear();
  m_version = 0;
  m_showRect = m2::RectD();
  m_zoomLevel = 0.0;
}

bool ParsedMapApi::GetViewport(m2::PointD & pt, double & zoom) const
{
  if (m_zoomLevel >= 1.0 && m_points.size() == 1)
  {
    zoom = min(static_cast<double>(scales::GetUpperComfortScale()), m_zoomLevel);
    pt.x = MercatorBounds::LonToX(m_points.front().m_lon);
    pt.y = MercatorBounds::LatToY(m_points.front().m_lat);
    return true;
  }

  return false;
}

bool ParsedMapApi::GetViewportRect(m2::RectD & rect) const
{
  if (m_showRect.IsValid())
  {
    rect = MercatorBounds::FromLatLonRect(m_showRect);
    return true;
  }
  else
    return false;
}
