#include "map/route_simulation.hpp"

#include "routing/routing_helpers.hpp"

#include "geometry/angles.hpp"
#include "geometry/distance_on_sphere.hpp"

#include "base/math.hpp"

RouteSimulation::RouteSimulation(routing::RouteJunctions const & junctions)
  : m_points(routing::DensifyJunctions(junctions, kRouteSimulationStepM))
{}

std::optional<location::GpsInfo> RouteSimulation::Next(double timestampS)
{
  if (m_next >= m_points.size())
    return std::nullopt;

  auto const & point = m_points[m_next];
  ms::LatLon const latLon = point.ToLatLon();

  location::GpsInfo info;
  info.m_source = location::EUser;
  info.m_latitude = latLon.m_lat;
  info.m_longitude = latLon.m_lon;
  info.m_horizontalAccuracy = 1.0;
  info.m_timestamp = timestampS;

  if (m_next > 0)
  {
    auto const & prev = m_points[m_next - 1];
    double const intervalS = std::chrono::duration<double>(kRouteSimulationInterval).count();
    info.m_speed = ms::DistanceOnEarth(prev.ToLatLon(), latLon) / intervalS;
    // The angle between identical points is meaningless.
    if (prev.GetPoint() != point.GetPoint())
      m_bearing = location::AngleToBearing(math::RadToDeg(ang::AngleTo(prev.GetPoint(), point.GetPoint())));
    info.m_bearing = m_bearing;
  }

  ++m_next;
  return info;
}
