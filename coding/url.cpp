#include "coding/url.hpp"

#include "coding/hex.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <cstddef>
#include <regex>
#include <sstream>
#include <vector>

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
  LatLonParser(url::Url const & url, url::GeoURLInfo & info)
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

  url::GeoURLInfo & m_info;
  url::Url const & m_url;
  regex m_regexp;
  int m_latPriority;
  int m_lonPriority;
};
}  // namespace

namespace url
{
std::string DebugPrint(Param const & param)
{
  return "UrlParam [" + param.m_name + "=" + param.m_value + "]";
}

Url::Url(std::string const & url)
{
  if (!Parse(url))
  {
    ASSERT(m_scheme.empty() && m_path.empty() && !IsValid(), ());
  }
}

bool Url::Parse(std::string const & url)
{
  // Get url scheme.
  size_t pathStart = url.find(':');
  if (pathStart == string::npos || pathStart == 0)
    return false;
  m_scheme.assign(url, 0, pathStart);

  // Skip slashes.
  while (++pathStart < url.size() && url[pathStart] == '/')
  {
  }

  // Find query starting point for (key, value) parsing.
  size_t queryStart = url.find('?', pathStart);
  size_t pathLength;
  if (queryStart == string::npos)
  {
    queryStart = url.size();
    pathLength = queryStart - pathStart;
  }
  else
  {
    pathLength = queryStart - pathStart;
    ++queryStart;
  }

  // Get path (url without query).
  m_path.assign(url, pathStart, pathLength);

  // Parse query for keys and values.
  for (size_t start = queryStart; start < url.size();)
  {
    size_t end = url.find('&', start);
    if (end == string::npos)
      end = url.size();

    // Skip empty keys.
    if (end != start)
    {
      size_t const eq = url.find('=', start);

      string key;
      string value;
      if (eq != string::npos && eq < end)
      {
        key = UrlDecode(url.substr(start, eq - start));
        value = UrlDecode(url.substr(eq + 1, end - eq - 1));
      }
      else
      {
        key = UrlDecode(url.substr(start, end - start));
      }

      m_params.emplace_back(key, value);
    }

    start = end + 1;
  }

  return true;
}

void Url::ForEachParam(Callback const & callback) const
{
  for (auto const & param : m_params)
    callback(param);
}

string Make(string const & baseUrl, Params const & params)
{
  ostringstream os;
  os << baseUrl;

  bool firstParam = baseUrl.find('?') == string::npos;
  for (auto const & param : params)
  {
    if (firstParam)
    {
      firstParam = false;
      os << "?";
    }
    else
    {
      os << "&";
    }

    os << param.m_name << "=" << param.m_value;
  }

  return os.str();
}

string Join(string const & lhs, string const & rhs)
{
  if (lhs.empty())
    return rhs;
  if (rhs.empty())
    return lhs;

  if (lhs.back() == '/' && rhs.front() == '/')
    return lhs + rhs.substr(1);

  if (lhs.back() != '/' && rhs.front() != '/')
    return lhs + '/' + rhs;

  return lhs + rhs;
}

string UrlEncode(string const & rawUrl)
{
  size_t const count = rawUrl.size();
  string result;
  result.reserve(count);

  for (size_t i = 0; i < count; ++i)
  {
    char const c = rawUrl[i];
    if (c < '-' || c == '/' || (c > '9' && c < 'A') || (c > 'Z' && c < '_') ||
        c == '`' || (c > 'z' && c < '~') || c > '~')
    {
      result += '%';
      result += NumToHex(c);
    }
    else
      result += rawUrl[i];
  }

  return result;
}

string UrlDecode(string const & encodedUrl)
{
  size_t const count = encodedUrl.size();
  string result;
  result.reserve(count);

  for (size_t i = 0; i < count; ++i)
  {
    if (encodedUrl[i] == '%')
    {
      result += FromHex(encodedUrl.substr(i + 1, 2));
      i += 2;
    }
    else
    {
      result += encodedUrl[i];
    }
  }

  return result;
}

GeoURLInfo::GeoURLInfo(string const & s)
{
  Reset();

  Url url(s);
  if (!url.IsValid())
    return;

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
}  // namespace url
