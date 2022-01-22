#include "geo_url_parser.hpp"

#include "geometry/mercator.hpp"

#include "coding/url.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

#include <regex>


namespace geo
{
using namespace std;

namespace
{
double const kInvalidCoord = -1000.0;

// Same as scales::GetUpperScale() from indexer/scales.hpp.
// 1. Duplicated here to avoid a dependency on indexer/.
// 2. The value is arbitrary anyway: we want to parse a "z=%f" pair
//    from a URL parameter, and different map providers may have different
//    maximal zoom levels.
double const kMaxZoom = 17.0;

class LatLonParser
{
public:
  LatLonParser(url::Url const & url, GeoURLInfo & info)
    : m_info(info)
    , m_url(url)
    , m_regexp("-?\\d+\\.{1}\\d*, *-?\\d+\\.{1}\\d*")
    , m_latPriority(-1)
    , m_lonPriority(-1)
  {
  }

  url::Url const & GetUrl() const { return m_url; }

  bool IsValid() const
  {
    return m_latPriority == m_lonPriority && m_latPriority != -1;
  }

  void operator()(url::Param const & param)
  {
    auto name = param.m_name;
    strings::AsciiToLower(name);
    if (name == "z" || name == "zoom")
    {
      double x;
      if (strings::to_double(param.m_value, x))
        m_info.SetZoom(x);
      return;
    }

    int const priority = GetCoordinatesPriority(name);
    if (priority == -1 || priority < m_latPriority || priority < m_lonPriority)
      return;

    if (priority != kXYPriority && priority != kLatLonPriority)
    {
      strings::ForEachMatched(param.m_value, m_regexp, AssignCoordinates(*this, priority));
      return;
    }

    double x;
    if (strings::to_double(param.m_value, x))
    {
      if (name == "lat" || name == "y")
      {
        if (!m_info.SetLat(x))
          return;
        m_latPriority = priority;
      }
      else
      {
        ASSERT(name == "lon" || name == "x", (param.m_name, name));
        if (!m_info.SetLon(x))
          return;
        m_lonPriority = priority;
      }
    }
  }

private:
  class AssignCoordinates
  {
  public:
    AssignCoordinates(LatLonParser & parser, int priority) : m_parser(parser), m_priority(priority)
    {
    }

    void operator()(string const & token) const
    {
      double lat;
      double lon;

      string::size_type n = token.find(',');
      if (n == string::npos)
        return;
      VERIFY(strings::to_double(token.substr(0, n), lat), ());

      n = token.find_first_not_of(", ", n);
      if (n == string::npos)
        return;
      VERIFY(strings::to_double(token.substr(n, token.size() - n), lon), ());

      SwapIfNeeded(lat, lon);

      if (m_parser.m_info.SetLat(lat) && m_parser.m_info.SetLon(lon))
      {
        m_parser.m_latPriority = m_priority;
        m_parser.m_lonPriority = m_priority;
      }
    }

    void SwapIfNeeded(double & lat, double & lon) const
    {
      vector<string> const kSwappingProviders = {"2gis", "yandex"};
      for (auto const & s : kSwappingProviders)
      {
        if (m_parser.GetUrl().GetPath().find(s) != string::npos)
        {
          swap(lat, lon);
          break;
        }
      }
    }

  private:
    LatLonParser & m_parser;
    int m_priority;
  };

  // Usually (lat, lon), but some providers use (lon, lat).
  inline static int const kLLPriority = 5;
  // We do not try to guess the projection and do not interpret (x, y)
  // as Mercator coordinates in URLs. We simply use (y, x) for (lat, lon).
  inline static int const kXYPriority = 6;
  inline static int const kLatLonPriority = 7;

  // Priority for accepting coordinates if we have many choices.
  // -1 - not initialized
  //  0 - coordinates in path;
  //  x - priority for query type (greater is better)
  int GetCoordinatesPriority(string const & token)
  {
    if (token.empty())
      return 0;
    if (token == "q" || token == "m")
      return 1;
    if (token == "saddr" || token == "daddr")
      return 2;
    if (token == "sll")
      return 3;
    if (token.find("point") != string::npos)
      return 4;
    if (token == "ll")
      return kLLPriority;
    if (token == "x" || token == "y")
      return kXYPriority;
    if (token == "lat" || token == "lon")
      return kLatLonPriority;

    return -1;
  }

  GeoURLInfo & m_info;
  url::Url const & m_url;
  regex m_regexp;
  int m_latPriority;
  int m_lonPriority;
}; // class LatLongParser


bool MatchLatLonZoom(const string & s, const regex & re, size_t lati, size_t loni, size_t zoomi, GeoURLInfo & info)
{
  std::smatch m;
  if (!std::regex_search(s, m, re) || m.size() != 4)
    return false;

  double lat;
  double lon;
  double zoom;
  VERIFY(strings::to_double(m[lati].str(), lat), ());
  VERIFY(strings::to_double(m[loni].str(), lon), ());
  VERIFY(strings::to_double(m[zoomi].str(), zoom), ());
  if (!info.SetLat(lat) || !info.SetLon(lon))
    return false;
  info.SetZoom(zoom);
  return true;
}

class DoubleGISParser
{
public:
  DoubleGISParser()
    : m_pathRe("/(\\d+\\.?\\d*),(\\d+\\.?\\d*)/zoom/(\\d+\\.?\\d*)"),
      m_paramRe("(\\d+\\.?\\d*),(\\d+\\.?\\d*)/(\\d+\\.?\\d*)")
  {
  }

  bool Parse(url::Url const & url, GeoURLInfo & info)
  {
    // Try m=$lon,$lat/$zoom first
    for (auto const & param : url.Params())
    {
      if (param.m_name == "m")
      {
        if (MatchLatLonZoom(param.m_value, m_paramRe, 2, 1, 3, info))
          return true;
        break;
      }
    }

    // Parse /$lon,$lat/zoom/$zoom from path next
    if (MatchLatLonZoom(url.GetPath(), m_pathRe, 2, 1, 3, info))
      return true;

    return false;
  }

private:
  regex m_pathRe;
  regex m_paramRe;
}; // Class DoubleGISParser

class OpenStreetMapParser
{
public:
  OpenStreetMapParser()
    : m_regex("#map=(\\d+\\.?\\d*)/(\\d+\\.\\d+)/(\\d+\\.\\d+)")
  {
  }

  bool Parse(url::Url const & url, GeoURLInfo & info)
  {
    if (MatchLatLonZoom(url.GetPath(), m_regex, 2, 3, 1, info))
      return true;
    // Check if "#map=" fragment is attached to the last param in Url
    if (!url.Params().empty() && MatchLatLonZoom(url.Params().back().m_value, m_regex, 2, 3, 1, info))
      return true;
    return false;
  }

private:
  regex m_regex;
}; // Class OpenStreetMapParser

}  // namespace


GeoURLInfo::GeoURLInfo()
{
}

void GeoURLInfo::Parse(string const & s)
{
  Reset();

  url::Url url(s);
  if (!url.IsValid())
    return;

  if (url.GetScheme() == "https" || url.GetScheme() == "http")
  {
    if (url.GetWebDomain().find("2gis") != string::npos)
    {
      DoubleGISParser parser;
      if (parser.Parse(url, *this))
        return;
    }
    else if (url.GetWebDomain().find("openstreetmap.org") != string::npos)
    {
      OpenStreetMapParser parser;
      if (parser.Parse(url, *this))
        return;
    }
  }

  LatLonParser parser(url, *this);
  parser(url::Param(string(), url.GetPath()));
  url.ForEachParam(ref(parser));

  if (!parser.IsValid())
  {
    Reset();
    return;
  }
}

bool GeoURLInfo::IsValid() const
{
  return m_lat != kInvalidCoord && m_lon != kInvalidCoord;
}

void GeoURLInfo::Reset()
{
  m_lat = kInvalidCoord;
  m_lon = kInvalidCoord;
  m_zoom = kMaxZoom;
}

void GeoURLInfo::SetZoom(double x)
{
  m_zoom = clamp(x, 0.0, kMaxZoom);
}

bool GeoURLInfo::SetLat(double x)
{
  if (mercator::ValidLat(x))
  {
    m_lat = x;
    return true;
  }
  return false;
}

bool GeoURLInfo::SetLon(double x)
{
  if (mercator::ValidLon(x))
  {
    m_lon = x;
    return true;
  }
  return false;
}
} // namespace geo
