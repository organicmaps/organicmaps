#include "geourl_process.hpp"

#include "../indexer/scales.hpp"
#include "../indexer/mercator.hpp"

#include "../base/string_utils.hpp"


namespace url_scheme
{
  bool Info::IsValid() const
  {
    return (MercatorBounds::ValidLat(m_lat) && MercatorBounds::ValidLon(m_lon));
  }

  void Info::Reset()
  {
    m_lat = m_lon = -1000.0;
    m_zoom = scales::GetUpperScale();
  }

  class DoGeoParse
  {
    Info & m_info;

    enum TMode { START, LAT, LON, ZOOM, TOKEN };
    TMode m_mode;

    bool CheckKeyword(string const & token)
    {
      if (token == "lat" || token == "point" || token == "q")
        m_mode = LAT;
      else if (token == "lon")
        m_mode = LON;
      else if (token == "zoom" || token == "z")
        m_mode = ZOOM;
      else
        return false;

      return true;
    }

    void CorrectZoomBounds(double & x)
    {
      if (x < 0.0)
        x = 0.0;
      int const upperScale = scales::GetUpperScale();
      if (x > upperScale)
        x = upperScale;
    }

  public:
    DoGeoParse(Info & info) : m_info(info), m_mode(START)
    {
    }

    bool operator()(string const & token)
    {
      // Check correct scheme and initialize mode.
      if (m_mode == START)
      {
        if (token != "geo")
          return false;
        else
        {
          m_mode = LAT;
          return true;
        }
      }

      // Check for any keyword.
      if (CheckKeyword(token))
        return true;
      else if (m_mode == TOKEN)
        return false;

      // Expect double value.
      double x;
      if (!strings::to_double(token, x))
        return false;

      // Assign value to the expected field.
      switch (m_mode)
      {
      case LAT:
        m_info.m_lat = x;
        m_mode = LON;
        break;

      case LON:
        m_info.m_lon = x;
        m_mode = TOKEN;
        break;

      case ZOOM:
        CorrectZoomBounds(x);
        m_info.m_zoom = x;
        m_mode = TOKEN;
        break;

      default:
        ASSERT(false, ());
        return false;
      }

      return true;
    }
  };

  void ParseGeoURL(string const & s, Info & info)
  {
    DoGeoParse parser(info);
    strings::SimpleTokenizer iter(s, ":/?&=, \t");

    while (iter && parser(*iter))
      ++iter;
  }
}
