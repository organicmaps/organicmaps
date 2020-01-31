#include "coding/url_helpers.hpp"

#include "coding/hex.hpp"

#include "geometry/mercator.hpp"

#include <cstddef>
#include <sstream>
#include <regex>

#include "base/assert.hpp"
#include "base/string_utils.hpp"

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
  explicit LatLonParser(coding::url::GeoURLInfo & info)
    : m_info(info)
    , m_regexp("-?\\d+\\.{1}\\d*, *-?\\d+\\.{1}\\d*")
    , m_latPriority(-1)
    , m_lonPriority(-1)
  {
  }

  bool IsValid() const
  {
    return m_latPriority == m_lonPriority && m_latPriority != -1;
  }

  bool operator()(string const & key, string const & value)
  {
    if (key == "z" || key == "zoom")
    {
      double x;
      if (strings::to_double(value, x))
        m_info.SetZoom(x);
      return true;
    }

    int const priority = GetCoordinatesPriority(key);
    if (priority == -1 || priority < m_latPriority || priority < m_lonPriority)
      return false;

    if (priority != kLatLonPriority)
    {
      strings::ForEachMatched(value, m_regexp, AssignCoordinates(*this, priority));
      return true;
    }

    double x;
    if (strings::to_double(value, x))
    {
      if (key == "lat")
      {
        if (!m_info.SetLat(x))
          return false;
        m_latPriority = priority;
      }
      else
      {
        ASSERT_EQUAL(key, "lon", ());
        if (!m_info.SetLon(x))
          return false;
        m_lonPriority = priority;
      }
    }
    return true;
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

      if (m_parser.m_info.SetLat(lat) && m_parser.m_info.SetLon(lon))
      {
        m_parser.m_latPriority = m_priority;
        m_parser.m_lonPriority = m_priority;
      }
    }

  private:
    LatLonParser & m_parser;
    int m_priority;
  };

  inline static int const kLatLonPriority = 5;

  // Priority for accepting coordinates if we have many choices.
  // -1 - not initialized
  //  0 - coordinates in path;
  //  x - priority for query type (greater is better)
  int GetCoordinatesPriority(string const & token)
  {
    if (token.empty())
      return 0;
    else if (token == "q")
      return 1;
    else if (token == "daddr")
      return 2;
    else if (token == "point")
      return 3;
    else if (token == "ll")
      return 4;
    else if (token == "lat" || token == "lon")
      return kLatLonPriority;

    return -1;
  }

  coding::url::GeoURLInfo & m_info;
  regex m_regexp;
  int m_latPriority;
  int m_lonPriority;
};
}  // namespace

namespace coding::url
{
Uri::Uri(std::string const & uri) : m_url(uri)
{
  if (!Parse())
  {
    ASSERT(m_scheme.empty() && m_path.empty() && !IsValid(), ());
    m_queryStart = m_url.size();
  }
}

bool Uri::Parse()
{
  // Get url scheme.
  size_t pathStart = m_url.find(':');
  if (pathStart == string::npos || pathStart == 0)
    return false;
  m_scheme.assign(m_url, 0, pathStart);

  // Skip slashes.
  while (++pathStart < m_url.size() && m_url[pathStart] == '/')
  {
  }

  // Find query starting point for (key, value) parsing.
  m_queryStart = m_url.find('?', pathStart);
  size_t pathLength;
  if (m_queryStart == string::npos)
  {
    m_queryStart = m_url.size();
    pathLength = m_queryStart - pathStart;
  }
  else
  {
    pathLength = m_queryStart - pathStart;
    ++m_queryStart;
  }

  // Get path (url without query).
  m_path.assign(m_url, pathStart, pathLength);

  return true;
}

bool Uri::ForEachKeyValue(Callback const & callback) const
{
  // Parse query for keys and values.
  size_t const count = m_url.size();
  size_t const queryStart = m_queryStart;

  // Just a URL without parameters.
  if (queryStart == count)
    return false;

  for (size_t start = queryStart; start < count; )
  {
    size_t end = m_url.find('&', start);
    if (end == string::npos)
      end = count;

    // Skip empty keys.
    if (end != start)
    {
      size_t const eq = m_url.find('=', start);

      string key, value;
      if (eq != string::npos && eq < end)
      {
        key = UrlDecode(m_url.substr(start, eq - start));
        value = UrlDecode(m_url.substr(eq + 1, end - eq - 1));
      }
      else
      {
        key = UrlDecode(m_url.substr(start, end - start));
      }

      if (!callback(key, value))
        return false;
    }

    start = end + 1;
  }
  return true;
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
  Uri uri(s);
  if (!uri.IsValid())
  {
    Reset();
    return;
  }

  LatLonParser parser(*this);
  parser(string(), uri.GetPath());
  uri.ForEachKeyValue(ref(parser));

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
  if (x >= 0.0 && x <= kMaxZoom)
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
}  // namespace coding::url
