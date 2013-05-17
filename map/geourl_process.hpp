#pragma once

#include "../geometry/rect2d.hpp"

#include "../std/string.hpp"


namespace url_scheme
{
  struct Info
  {
    double m_lat, m_lon, m_zoom;

    bool IsValid() const;
    void Reset();

    Info()
    {
      Reset();
    }

    m2::RectD GetViewport() const;
    /// @return lat and lon in Mercator projection
    m2::PointD GetMercatorPoint() const;
  };

  void ParseGeoURL(string const & s, Info & info);
}
