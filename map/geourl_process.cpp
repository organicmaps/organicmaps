#include "geourl_process.hpp"

#include "../indexer/scales.hpp"
#include "../indexer/mercator.hpp"

#include "../coding/uri.hpp"

#include "../base/string_utils.hpp"
#include "../base/regexp.hpp"
#include "../base/logging.hpp"

#include "../std/bind.hpp"


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
    else if (token == "point")
      return 2;
    else if (token == "ll")
      return 3;
    else if (token == "lat" || token == "lon")
      return 4;

    return -1;
  }

  class LatLonParser
  {
    regexp::RegExpT m_regexp;
    Info & m_info;
    int m_pLat, m_pLon;

    class AssignCoordinates
    {
      LatLonParser & m_parser;
      int m_p;
    public:
      AssignCoordinates(LatLonParser & parser, int p)
        : m_parser(parser), m_p(p)
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
          m_parser.m_pLat = m_parser.m_pLon = m_p;
      }
    };

  public:
    LatLonParser(Info & info) : m_info(info), m_pLat(-1), m_pLon(-1)
    {
      regexp::Create("-?\\d+\\.?\\d*, *-?\\d+\\.?\\d*", m_regexp);
    }

    bool IsValid() const
    {
      return (m_pLat == m_pLon && m_pLat != -1);
    }

    void operator()(string const & key, string const & value)
    {
      if (key == "z" || key == "zoom")
      {
        double x;
        if (strings::to_double(value, x))
          m_info.SetZoom(x);
        return;
      }

      int const p = GetCoordinatesPriority(key);
      if (p == -1 || p < m_pLat || p < m_pLon)
        return;

      if (p != 4)
      {
        regexp::ForEachMatched(value, m_regexp, AssignCoordinates(*this, p));
      }
      else
      {
        double x;
        if (strings::to_double(value, x))
        {
          if (key == "lat")
          {
            if (m_info.SetLat(x))
              m_pLat = p;
          }
          else
          {
            ASSERT_EQUAL(key, "lon", ());
            if (m_info.SetLon(x))
              m_pLon = p;
          }
        }
      }
    }
  };

  void ParseGeoURL(string const & s, Info & info)
  {
    url_scheme::Uri uri(s);
    if (!uri.IsValid())
      return;

    LatLonParser parser(info);
    parser(string(), uri.GetPath());
    uri.ForEachKeyValue(bind<void>(ref(parser), _1, _2));

    if (!parser.IsValid())
      info.Reset();
  }
}
