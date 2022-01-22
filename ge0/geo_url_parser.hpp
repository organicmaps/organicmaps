#pragma once

#include <string>

namespace geo
{

class GeoURLInfo
{
public:
  GeoURLInfo();

  void Parse(std::string const & s);

  bool IsValid() const;
  void Reset();

  void SetZoom(double x);
  bool SetLat(double x);
  bool SetLon(double x);

  double m_lat;
  double m_lon;
  double m_zoom;
};

} // namespace geo
