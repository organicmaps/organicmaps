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
  explicit LatLonWithAltitude(ms::LatLon const & latlon) : m_latlon(latlon) {}
  LatLonWithAltitude(ms::LatLon const & latlon, geometry::Altitude altitude, bool hasAltitude)
    : m_latlon(latlon)
    , m_altitude(altitude)
    , m_hasAltitude(hasAltitude)
  {}
  explicit LatLonWithAltitude(geometry::PointWithAltitude const & point);

  bool operator==(LatLonWithAltitude const & rhs) const;
  bool operator<(LatLonWithAltitude const & rhs) const;

  ms::LatLon const & GetLatLon() const { return m_latlon; }
  geometry::Altitude GetAltitude() const { return m_altitude; }
  bool HasAltitude() const { return m_hasAltitude; }
  geometry::Altitude GetRouteAltitude() const { return m_hasAltitude ? m_altitude : geometry::kInvalidAltitude; }

  // Retain numeric fallback heights for graph and directions lookups.
  geometry::PointWithAltitude ToPointWithAltitude() const;

private:
  ms::LatLon m_latlon;
  geometry::Altitude m_altitude = geometry::kDefaultAltitudeMeters;
  bool m_hasAltitude = false;
};

std::string DebugPrint(LatLonWithAltitude const & latLonWithAltitude);
}  // namespace routing
