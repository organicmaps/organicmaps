#pragma once

#include "routing/route.hpp"

#include "platform/location.hpp"

#include <chrono>
#include <cstddef>
#include <optional>

// 50 km/h at one fix per second.
double constexpr kRouteSimulationStepM = 13.9;
std::chrono::seconds constexpr kRouteSimulationInterval{1};

class RouteSimulation
{
public:
  explicit RouteSimulation(routing::RouteJunctions const & junctions);

  /// \returns std::nullopt after the last route point.
  std::optional<location::GpsInfo> Next(double timestampS);

private:
  routing::RouteJunctions m_points;
  size_t m_next = 0;
  double m_bearing = -1.0;
};
