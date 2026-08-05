#pragma once

#include "routing/vehicle_mask.hpp"

namespace routing
{
struct RouteSpeedSettings
{
  double m_cruisingSpeedKMpH = 0.0;
  int m_windSpeedMpS = 0;
  int m_windDirectionDegrees = 0;

  bool operator==(RouteSpeedSettings const &) const = default;
};

struct CruisingSpeedRange
{
  double m_min;
  double m_max;
  double m_step;
  double m_default;
};

int constexpr kMaxWindSpeedMpS = 20;
int constexpr kWindDirectionStepDegrees = 45;

/// Only muscle-powered routers have a personal speed, and only cyclists are noticeably slowed by wind.
bool IsRouteSpeedSupported(VehicleType vehicleType);
bool IsWindSupported(VehicleType vehicleType);

CruisingSpeedRange GetCruisingSpeedRange(VehicleType vehicleType);
RouteSpeedSettings LoadRouteSpeedSettings(VehicleType vehicleType);
void SaveRouteSpeedSettings(VehicleType vehicleType, RouteSpeedSettings const & settings);
}  // namespace routing
