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
double constexpr kEps = 1e-10;

// Same as scales::GetUpperScale() from indexer/scales.hpp.
// 1. Duplicated here to avoid a dependency on indexer/.
// 2. The value is arbitrary anyway: we want to parse a "z=%f" pair
//    from a URL parameter, and different map providers may have different
//    maximal zoom levels.
double constexpr kMaxZoom = 20.0;

bool MatchLatLonZoom(string const & s, regex const & re, size_t lati, size_t loni, size_t zoomi, GeoURLInfo & info)
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
}  // namespace

LatLonParser::LatLonParser() : m_info(nullptr), m_regexp(kFloatCoord + ", *" + kFloatCoord) {}

void LatLonParser::Reset(url::Url const & url, GeoURLInfo & info)
{
  info.Reset();
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
    if (name == "lat" || name == "mlat" || name == "y")
    {
      if (m_info->SetLat(x))
        m_latPriority = priority;
    }
    else
    {
      ASSERT(name == "lon" || name == "mlon" || name == "x", (name));
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
  if (token == "lat" || token == "mlat" || token == "lon" || token == "mlon")
    return kLatLonPriority;

  return -1;
}

std::string const kLatLon = R"(([+-]?\d+(?:\.\d+)?), *([+-]?\d+(?:\.\d+)?)(:?, *([+-]?\d+(?:\.\d+)?))?)";

GeoParser::GeoParser() : m_latlonRe(kLatLon), m_zoomRe(kFloatIntScale) {}

bool GeoParser::Parse(std::string const & raw, GeoURLInfo & info) const
{
  info.Reset();

  /*
   * References:
   * - https://datatracker.ietf.org/doc/html/rfc5870
   * - https://developer.android.com/guide/components/intents-common#Maps
   * - https://developers.google.com/maps/documentation/urls/android-intents
   */

  /*
   * Check that URI starts with geo:
   */
  if (!raw.starts_with("geo:"))
    return false;

  /*
   * Check for trailing `(label)` which is not RFC3986-compliant (thanks, Google).
   */
  size_t end = string::npos;
  if (raw.size() > 2 && raw.back() == ')' && string::npos != (end = raw.rfind('(')))
  {
    // head (label)
    //      ^end
    info.m_label = url::UrlDecode(raw.substr(end + 1, raw.size() - end - 2));
    // Remove any whitespace between `head` and `(`.
    end--;
    while (end > 0 && (raw[end] == ' ' || raw[end] == '+'))
      end--;
  }

  url::Url url(end == string::npos ? raw : raw.substr(0, end + 1));
  if (!url.IsValid())
    return false;
  ASSERT_EQUAL(url.GetScheme(), "geo", ());

  // Fix non-RFC url/hostname, reported by an Android user, with & instead of ?
  std::string_view constexpr kWrongZoomInHost = "&z=";
  if (std::string::npos != url.GetHost().find(kWrongZoomInHost))
  {
    auto fixedUrl = raw;
    fixedUrl.replace(raw.find(kWrongZoomInHost), 1, 1, std::string::value_type{'?'});
    url = url::Url{fixedUrl};
  }

  /*
   * Parse coordinates before ';' character
   */
  std::string coordinates = url.GetHost().substr(0, url.GetHost().find(';'));
  if (!coordinates.empty())
  {
    std::smatch m;
    if (!std::regex_match(coordinates, m, m_latlonRe) || m.size() < 3)
    {
      // no match? try URL decoding before giving up
      coordinates = url::UrlDecode(coordinates);
      if (!std::regex_match(coordinates, m, m_latlonRe) || m.size() < 3)
      {
        LOG(LWARNING, ("Missing coordinates in", raw));
        return false;
      }
    }

    double lat, lon;
    VERIFY(strings::to_double(m[1].str(), lat), ());
    VERIFY(strings::to_double(m[2].str(), lon), ());
    if (!mercator::ValidLat(lat) || !mercator::ValidLon(lon))
    {
      LOG(LWARNING, ("Invalid lat,lon in", raw));
      return false;
    }
    info.m_lat = lat;
    info.m_lon = lon;
  }

  /*
   * Parse q=
   */
  std::string const * q = url.GetParamValue("q");
  if (q != nullptr && !q->empty())
  {
    // Try to extract lat,lon from q=
    std::smatch m;
    if (std::regex_match(*q, m, m_latlonRe) && m.size() != 3)
    {
      double lat, lon;
      VERIFY(strings::to_double(m[1].str(), lat), ());
      VERIFY(strings::to_double(m[2].str(), lon), ());
      if (!mercator::ValidLat(lat) || !mercator::ValidLon(lon))
      {
        LOG(LWARNING, ("Invalid lat,lon after q=", raw));
        info.m_query = *q;
      }
      else
      {
        info.m_lat = lat;
        info.m_lon = lon;
      }
    }
    else
    {
      info.m_query = *q;
    }
    // Ignore special 0,0 lat,lon if q= presents.
    if (!info.m_query.empty() && fabs(info.m_lat) < kEps && fabs(info.m_lon) < kEps)
      info.m_lat = info.m_lon = ms::LatLon::kInvalid;
  }

  if (!info.IsLatLonValid() && info.m_query.empty())
  {
    LOG(LWARNING, ("Missing coordinates and q=", raw));
    return false;
  }

  /*
   * Parse z=
   */
  std::string const * z = url.GetParamValue("z");
  if (z != nullptr)
  {
    std::smatch m;
    if (std::regex_match(*z, m, m_zoomRe) && m.size() == 2)
    {
      double zoom;
      VERIFY(strings::to_double(m[0].str(), zoom), ());
      info.SetZoom(zoom);
    }
    else
    {
      LOG(LWARNING, ("Invalid z=", *z));
    }
  }

  return true;
}

DoubleGISParser::DoubleGISParser()
  : m_pathRe("/" + kFloatIntCoord + "," + kFloatIntCoord + "/zoom/" + kFloatIntScale)
  , m_paramRe(kFloatIntCoord + "," + kFloatIntCoord + "/" + kFloatIntScale)
{}

bool DoubleGISParser::Parse(url::Url const & url, GeoURLInfo & info) const
{
  info.Reset();

  // Try m=$lon,$lat/$zoom first
  auto const * value = url.GetParamValue("m");
  if (value && MatchLatLonZoom(*value, m_paramRe, 2, 1, 3, info))
    return true;

  // Parse /$lon,$lat/zoom/$zoom from path next
  return MatchLatLonZoom(url.GetHostAndPath(), m_pathRe, 2, 1, 3, info);
}

OpenStreetMapParser::OpenStreetMapParser() : m_regex(kIntScale + "/" + kFloatCoord + "/" + kFloatCoord) {}

bool OpenStreetMapParser::Parse(url::Url const & url, GeoURLInfo & info) const
{
  info.Reset();

  auto const * mapV = url.GetParamValue("map");
  return (mapV && MatchLatLonZoom(*mapV, m_regex, 2, 3, 1, info));
}

GeoURLInfo::GeoURLInfo()
{
  Reset();
}

bool GeoURLInfo::IsLatLonValid() const
{
  return m_lat != ms::LatLon::kInvalid && m_lon != ms::LatLon::kInvalid;
}

void GeoURLInfo::Reset()
{
  m_lat = ms::LatLon::kInvalid;
  m_lon = ms::LatLon::kInvalid;
  m_zoom = 0.0;
  m_query = "";
  m_label = "";
}

void GeoURLInfo::SetZoom(double x)
{
  if (x < 1.0)
    LOG(LWARNING, ("Invalid zoom:", x));
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

bool UnifiedParser::Parse(std::string const & raw, GeoURLInfo & res)
{
  if (raw.starts_with("geo:"))
    return m_geoParser.Parse(raw, res);

  url::Url url(raw);
  if (!url.IsValid())
    return false;

  if (url.GetScheme() != "https" && url.GetScheme() != "http")
    return false;

  if (MatchHost(url, "2gis") && m_dgParser.Parse(url, res))
    return true;
  else if (MatchHost(url, "openstreetmap") && m_osmParser.Parse(url, res))
    return true;
  // Fall through.

  m_llParser.Reset(url, res);
  m_llParser({}, url.GetHostAndPath());
  url.ForEachParam(m_llParser);
  return m_llParser.IsValid();
}

}  // namespace geo
