#pragma once

#include "geometry/rect2d.hpp"

#include "std/string.hpp"


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

    void SetZoom(double x);
    bool SetLat(double x);
    bool SetLon(double x);
  };

  void ParseGeoURL(string const & s, Info & info);
}
