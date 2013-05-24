#include "mwm_url.hpp"

#include "../indexer/mercator.hpp"

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

ParsedMapApi::ParsedMapApi():m_id(0)
{}

ParsedMapApi::ParsedMapApi(Uri const & uri):m_id(0)
{
  if (!Parse(uri))
  {
    m_points.clear();
  }
}

void ParsedMapApi::SetUriAndParse(string const & url)
{
  Clear();
  Parse(url_scheme::Uri(url));
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

void ParsedMapApi::AddKeyValue(string const & key, string const & value)
{
  if (key == "backurl")
    m_globalBackUrl = value;
  if (key == "v")
  {
    if (!strings::to_int(value, m_id))
      m_id = 0;
  }
  if (key == "appname")
    m_appTitle = value;
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
    m_showRect = m2::Add(m_showRect, m2::PointD(lat, lon));
  }
  else if (key == "n")
  {
    if (!m_points.empty())
      m_points.back().m_title = value;
    else
      LOG(LWARNING, ("Map API: Point name with no point. 'll' should come first!"));
  }
  else if (key == "u")
  {
    if (!m_points.empty())
      m_points.back().m_url = value;
    else
      LOG(LWARNING, ("Map API: Point url with no point. 'll' should come first!"));
  }
}

void ParsedMapApi::Clear()
{
  m_points.clear();
  m_globalBackUrl.clear();
  m_appTitle.clear();
  m_id = 0;
  m_showRect = m2::RectD();
}
