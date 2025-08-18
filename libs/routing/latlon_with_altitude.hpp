#pragma once

#include "geometry/latlon.hpp"
#include "geometry/point_with_altitude.hpp"

#include <string>

namespace routing
{
class LatLonWithAltitude
{
public:
  LatLonWithAltitude() = default;
  LatLonWithAltitude(ms::LatLon const & latlon, geometry::Altitude altitude) : m_latlon(latlon), m_altitude(altitude) {}

  bool operator==(LatLonWithAltitude const & rhs) const;
  bool operator<(LatLonWithAltitude const & rhs) const;

  ms::LatLon const & GetLatLon() const { return m_latlon; }
  geometry::Altitude GetAltitude() const { return m_altitude; }

  geometry::PointWithAltitude ToPointWithAltitude() const;

private:
  ms::LatLon m_latlon;
  geometry::Altitude m_altitude = geometry::kDefaultAltitudeMeters;
};

std::string DebugPrint(LatLonWithAltitude const & latLonWithAltitude);
}  // namespace routing
