#include "geo_url_parser.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

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

bool EqualWebDomain(url::Url const & url, char const * domain)
{
  return url.GetWebDomain().find(domain) != std::string::npos;
}
} // namespace

LatLonParser::LatLonParser()
: m_info(nullptr), m_url(nullptr)
, m_regexp("-?\\d+\\.{1}\\d*, *-?\\d+\\.{1}\\d*")
{
}

void LatLonParser::Reset(url::Url const & url, GeoURLInfo & info)
{
  m_info = &info;
  m_url = &url;
  m_latPriority = m_lonPriority = -1;
}

bool LatLonParser::IsValid() const
{
  return m_latPriority == m_lonPriority && m_latPriority != -1;
}

void LatLonParser::operator()(url::Param const & param)
{
  auto name = param.m_name;
  strings::AsciiToLower(name);
  if (name == "z" || name == "zoom")
  {
    double x;
    if (strings::to_double(param.m_value, x))
      m_info->SetZoom(x);
    return;
  }

  int const priority = GetCoordinatesPriority(name);
  if (priority == -1 || priority < m_latPriority || priority < m_lonPriority)
    return;

  if (priority != kXYPriority && priority != kLatLonPriority)
  {
    strings::ForEachMatched(param.m_value, m_regexp, [this, priority](string const & token)
    {
      double lat;
      double lon;

      size_t n = token.find(',');
      if (n == string::npos)
        return;
      VERIFY(strings::to_double(token.substr(0, n), lat), ());

      n = token.find_first_not_of(", ", n);
      if (n == string::npos)
        return;
      VERIFY(strings::to_double(token.substr(n, token.size() - n), lon), ());

      if (EqualWebDomain(*m_url, "2gis") || EqualWebDomain(*m_url, "yandex"))
        std::swap(lat, lon);

      if (m_info->SetLat(lat) && m_info->SetLon(lon))
      {
        m_latPriority = priority;
        m_lonPriority = priority;
      }
    });

    return;
  }

  double x;
  if (strings::to_double(param.m_value, x))
  {
    if (name == "lat" || name == "y")
    {
      if (m_info->SetLat(x))
        m_latPriority = priority;
    }
    else
    {
      ASSERT(name == "lon" || name == "x", (param.m_name, name));
      if (m_info->SetLon(x))
        m_lonPriority = priority;
    }
  }
}

int LatLonParser::GetCoordinatesPriority(string const & token)
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

DoubleGISParser::DoubleGISParser()
: m_pathRe("/(\\d+\\.?\\d*),(\\d+\\.?\\d*)/zoom/(\\d+\\.?\\d*)")
, m_paramRe("(\\d+\\.?\\d*),(\\d+\\.?\\d*)/(\\d+\\.?\\d*)")
{
}

bool DoubleGISParser::Parse(url::Url const & url, GeoURLInfo & info) const
{
  // Try m=$lon,$lat/$zoom first
  auto const * value = url.GetParamValue("m");
  if (value && MatchLatLonZoom(*value, m_paramRe, 2, 1, 3, info))
    return true;

  // Parse /$lon,$lat/zoom/$zoom from path next
  return MatchLatLonZoom(url.GetPath(), m_pathRe, 2, 1, 3, info);
}

OpenStreetMapParser::OpenStreetMapParser()
: m_regex("#map=(\\d+\\.?\\d*)/(\\d+\\.\\d+)/(\\d+\\.\\d+)")
{
}

bool OpenStreetMapParser::Parse(url::Url const & url, GeoURLInfo & info) const
{
  if (MatchLatLonZoom(url.GetPath(), m_regex, 2, 3, 1, info))
    return true;

  // Check if "#map=" fragment is attached to the last param in Url
  auto const * last = url.GetLastParam();
  return (last && MatchLatLonZoom(last->m_value, m_regex, 2, 3, 1, info));
}

GeoURLInfo::GeoURLInfo()
{
  Reset();
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

GeoURLInfo UnifiedParser::Parse(string const & s)
{
  GeoURLInfo res;

  url::Url url(s);
  if (!url.IsValid())
    return res;

  if (url.GetScheme() == "https" || url.GetScheme() == "http")
  {
    if (EqualWebDomain(url, "2gis"))
    {
      if (m_dgParser.Parse(url, res))
        return res;
    }
    else if (EqualWebDomain(url, "openstreetmap"))
    {
      if (m_osmParser.Parse(url, res))
        return res;
    }
  }

  m_llParser.Reset(url, res);
  m_llParser({{}, url.GetPath()});
  url.ForEachParam(m_llParser);

  if (!m_llParser.IsValid())
    res.Reset();

  return res;
}

} // namespace geo
