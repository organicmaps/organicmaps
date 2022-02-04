#include "geo_url_parser.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

namespace geo
{
using namespace std;

namespace
{
double constexpr kInvalidCoord = -1000.0;

// Same as scales::GetUpperScale() from indexer/scales.hpp.
// 1. Duplicated here to avoid a dependency on indexer/.
// 2. The value is arbitrary anyway: we want to parse a "z=%f" pair
//    from a URL parameter, and different map providers may have different
//    maximal zoom levels.
double constexpr kMaxZoom = 17.0;
double constexpr kDefaultZoom = 15.0;

bool MatchLatLonZoom(string const & s, regex const & re,
                     size_t lati, size_t loni, size_t zoomi,
                     GeoURLInfo & info)
{
  std::smatch m;
  if (!std::regex_search(s, m, re) || m.size() != 4)
    return false;

  double lat, lon, zoom;
  VERIFY(strings::to_double(m[lati].str(), lat), ());
  VERIFY(strings::to_double(m[loni].str(), lon), ());
  VERIFY(strings::to_double(m[zoomi].str(), zoom), ());
  if (info.SetLat(lat) && info.SetLon(lon))
  {
    info.SetZoom(zoom);
    return true;
  }

  LOG(LWARNING, ("Bad coordinates url:", s));
  return false;
}

bool MatchHost(url::Url const & url, char const * host)
{
  return url.GetHost().find(host) != std::string::npos;
}

// Canonical parsing is float-only coordinates and int-only scale.
std::string const kFloatCoord = R"(([+-]?\d+\.\d+))";
std::string const kIntScale = R"((\d+))";

// 2gis can accept float or int coordinates and scale.
std::string const kFloatIntCoord = R"(([+-]?\d+\.?\d*))";
std::string const kFloatIntScale = R"((\d+\.?\d*))";
} // namespace

LatLonParser::LatLonParser()
: m_info(nullptr)
, m_regexp(kFloatCoord + ", *" + kFloatCoord)
{
}

void LatLonParser::Reset(url::Url const & url, GeoURLInfo & info)
{
  m_info = &info;
  m_swapLatLon = MatchHost(url, "2gis") || MatchHost(url, "yandex");
  m_latPriority = m_lonPriority = -1;
}

bool LatLonParser::IsValid() const
{
  return m_latPriority == m_lonPriority && m_latPriority != -1;
}

void LatLonParser::operator()(std::string name, std::string const & value)
{
  strings::AsciiToLower(name);
  if (name == "z" || name == "zoom")
  {
    double x;
    if (strings::to_double(value, x))
      m_info->SetZoom(x);
    return;
  }

  int const priority = GetCoordinatesPriority(name);
  if (priority == -1 || priority < m_latPriority || priority < m_lonPriority)
    return;

  if (priority != kXYPriority && priority != kLatLonPriority)
  {
    std::smatch m;
    if (std::regex_search(value, m, m_regexp) && m.size() == 3)
    {
      double lat, lon;
      VERIFY(strings::to_double(m[1].str(), lat), ());
      VERIFY(strings::to_double(m[2].str(), lon), ());

      if (m_swapLatLon)
        std::swap(lat, lon);

      if (m_info->SetLat(lat) && m_info->SetLon(lon))
      {
        m_latPriority = priority;
        m_lonPriority = priority;
      }
    }

    return;
  }

  double x;
  if (strings::to_double(value, x))
  {
    if (name == "lat" || name == "y")
    {
      if (m_info->SetLat(x))
        m_latPriority = priority;
    }
    else
    {
      ASSERT(name == "lon" || name == "x", (name));
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
: m_pathRe("/" + kFloatIntCoord + "," + kFloatIntCoord + "/zoom/" + kFloatIntScale)
, m_paramRe(kFloatIntCoord + "," + kFloatIntCoord + "/" + kFloatIntScale)
{
}

bool DoubleGISParser::Parse(url::Url const & url, GeoURLInfo & info) const
{
  // Try m=$lon,$lat/$zoom first
  auto const * value = url.GetParamValue("m");
  if (value && MatchLatLonZoom(*value, m_paramRe, 2, 1, 3, info))
    return true;

  // Parse /$lon,$lat/zoom/$zoom from path next
  return MatchLatLonZoom(url.GetHostAndPath(), m_pathRe, 2, 1, 3, info);
}

OpenStreetMapParser::OpenStreetMapParser()
: m_regex(kIntScale + "/" + kFloatCoord + "/" + kFloatCoord)
{
}

bool OpenStreetMapParser::Parse(url::Url const & url, GeoURLInfo & info) const
{
  auto const * mapV = url.GetParamValue("map");
  return (mapV && MatchLatLonZoom(*mapV, m_regex, 2, 3, 1, info));
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
  if (x < 1.0)
    m_zoom = kDefaultZoom;
  else if (x > kMaxZoom)
    m_zoom = kMaxZoom;
  else
    m_zoom = x;
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
    if (MatchHost(url, "2gis"))
    {
      if (m_dgParser.Parse(url, res))
        return res;
    }
    else if (MatchHost(url, "openstreetmap"))
    {
      if (m_osmParser.Parse(url, res))
        return res;
    }
  }

  m_llParser.Reset(url, res);
  m_llParser({}, url.GetHostAndPath());
  url.ForEachParam(m_llParser);

  if (!m_llParser.IsValid())
    res.Reset();

  return res;
}

} // namespace geo
