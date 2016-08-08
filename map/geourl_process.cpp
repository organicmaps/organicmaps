#include "map/geourl_process.hpp"

#include "geometry/mercator.hpp"
#include "indexer/scales.hpp"

#include "coding/uri.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "std/bind.hpp"
#include "std/regex.hpp"

namespace url_scheme
{
  const double INVALID_COORD = -1000.0;

  bool Info::IsValid() const
  {
    return (m_lat != INVALID_COORD && m_lon != INVALID_COORD);
  }

  void Info::Reset()
  {
    m_lat = m_lon = INVALID_COORD;
    m_zoom = scales::GetUpperScale();
  }

  void Info::SetZoom(double x)
  {
    if (x >= 0.0 && x <= scales::GetUpperScale())
      m_zoom = x;
  }

  bool Info::SetLat(double x)
  {
    if (MercatorBounds::ValidLat(x))
    {
      m_lat = x;
      return true;
    }
    return false;
  }

  bool Info::SetLon(double x)
  {
    if (MercatorBounds::ValidLon(x))
    {
      m_lon = x;
      return true;
    }
    return false;
  }

  int const LL_PRIORITY = 5;

  /// Priority for accepting coordinates if we have many choices.
  /// -1 - not initialized
  /// 0 - coordinates in path;
  /// x - priority for query type (greater is better)
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
      return LL_PRIORITY;

    return -1;
  }

  class LatLonParser
  {
    regex m_regexp;
    Info & m_info;
    int m_latPriority, m_lonPriority;

    class AssignCoordinates
    {
      LatLonParser & m_parser;
      int m_priority;

    public:
      AssignCoordinates(LatLonParser & parser, int priority)
        : m_parser(parser), m_priority(priority)
      {
      }

      void operator() (string const & token) const
      {
        double lat, lon;

        string::size_type n = token.find(',');
        ASSERT(n != string::npos, ());
        VERIFY(strings::to_double(token.substr(0, n), lat), ());

        n = token.find_first_not_of(", ", n);
        ASSERT(n != string::npos, ());
        VERIFY(strings::to_double(token.substr(n, token.size() - n), lon), ());

        if (m_parser.m_info.SetLat(lat) && m_parser.m_info.SetLon(lon))
          m_parser.m_latPriority = m_parser.m_lonPriority = m_priority;
      }
    };

  public:
    LatLonParser(Info & info)
      : m_regexp("-?\\d+\\.{1}\\d*, *-?\\d+\\.{1}\\d*")
      , m_info(info)
      , m_latPriority(-1)
      , m_lonPriority(-1)
    {
    }

    bool IsValid() const
    {
      return (m_latPriority == m_lonPriority && m_latPriority != -1);
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

      if (priority != LL_PRIORITY)
      {
        strings::ForEachMatched(value, m_regexp, AssignCoordinates(*this, priority));
      }
      else
      {
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
      }
      return true;
    }
  };

  void ParseGeoURL(string const & s, Info & info)
  {
    url_scheme::Uri uri(s);
    if (!uri.IsValid())
      return;

    LatLonParser parser(info);
    parser(string(), uri.GetPath());
    uri.ForEachKeyValue(ref(parser));

    if (!parser.IsValid())
      info.Reset();
  }
}
