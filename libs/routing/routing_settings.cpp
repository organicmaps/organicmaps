#include "routing/routing_settings.hpp"

#include "routing/routing_helpers.hpp"

#include "base/assert.hpp"

namespace routing
{
RoutingSettings GetRoutingSettings(VehicleType vehicleType)
{
  switch (vehicleType)
  {
  case VehicleType::Pedestrian:
    return {.m_useDirectionForRouteBuilding = false,
            .m_matchRoute = false,
            .m_soundDirection = true,
            .m_matchingThresholdM = 20.0,
            .m_showTurnAfterNext = false,
            .m_minSpeedForRouteRebuildMpS = -1,
            .m_finishToleranceM = 15.0,
            .m_maxOutgoingPointsCount = 6,
            .m_minOutgoingDistMeters = 5.0,
            .m_maxIngoingPointsCount = 2,
            .m_minIngoingDistMeters = 4.0};
  case VehicleType::Transit:
    return {.m_useDirectionForRouteBuilding = false,
            .m_matchRoute = true,
            .m_soundDirection = false,
            .m_matchingThresholdM = 40.0,
            .m_showTurnAfterNext = false,
            .m_minSpeedForRouteRebuildMpS = -1,
            .m_finishToleranceM = 15.0,
            .m_maxOutgoingPointsCount = 6,
            .m_minOutgoingDistMeters = 5.0,
            .m_maxIngoingPointsCount = 2,
            .m_minIngoingDistMeters = 4.0};
  case VehicleType::Bicycle:
    return {.m_useDirectionForRouteBuilding = false,
            .m_matchRoute = true,
            .m_soundDirection = true,
            .m_matchingThresholdM = 30.0,
            .m_showTurnAfterNext = false,
            .m_minSpeedForRouteRebuildMpS = -1,
            .m_finishToleranceM = 15.0,
            .m_maxOutgoingPointsCount = 9,
            .m_minOutgoingDistMeters = 10.0,
            .m_maxIngoingPointsCount = 2,
            .m_minIngoingDistMeters = 10.0};
  case VehicleType::Car:
    return {.m_useDirectionForRouteBuilding = true,
            .m_matchRoute = true,
            .m_soundDirection = true,
            .m_matchingThresholdM = 50.0,
            .m_showTurnAfterNext = true,
            .m_minSpeedForRouteRebuildMpS = measurement_utils::KmphToMps(3.0),
            .m_finishToleranceM = 20.0,
            .m_maxOutgoingPointsCount = 9,
            .m_minOutgoingDistMeters = 120.0,
            .m_maxIngoingPointsCount = 2,
            .m_minIngoingDistMeters = 100.0};
  case VehicleType::Count: CHECK(false, ("Can't create GetRoutingSettings for", vehicleType));
  }
  UNREACHABLE();
}
}  // namespace routing
