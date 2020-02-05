#include "routing/routing_settings.hpp"

#include "routing/routing_helpers.hpp"

#include "base/assert.hpp"

namespace routing
{
// RoutingSettings ---------------------------------------------------------------------------------
RoutingSettings::RoutingSettings(bool useDirectionForRouteBuilding, bool matchRoute,
                                 bool soundDirection, double matchingThresholdM,
                                 bool showTurnAfterNext,
                                 double minSpeedForRouteRebuildMpS, double finishToleranceM)

  : m_useDirectionForRouteBuilding(useDirectionForRouteBuilding)
  , m_matchRoute(matchRoute)
  , m_soundDirection(soundDirection)
  , m_matchingThresholdM(matchingThresholdM)
  , m_showTurnAfterNext(showTurnAfterNext)
  , m_minSpeedForRouteRebuildMpS(minSpeedForRouteRebuildMpS)
  , m_finishToleranceM(finishToleranceM)
{
}

RoutingSettings GetRoutingSettings(VehicleType vehicleType)
{
  switch (vehicleType)
  {
  case VehicleType::Pedestrian:
    return {false /* useDirectionForRouteBuilding */,
            false /* m_matchRoute */,
            false /* m_soundDirection */,
            20.0 /* m_matchingThresholdM */,
            false /* m_showTurnAfterNext */,
            -1 /* m_minSpeedForRouteRebuildMpS */,
            15.0 /* m_finishToleranceM */};
  case VehicleType::Transit:
    return {false /* useDirectionForRouteBuilding */,
            true /* m_matchRoute */,
            false /* m_soundDirection */,
            40.0 /* m_matchingThresholdM */,
            false /* m_showTurnAfterNext */,
            -1 /* m_minSpeedForRouteRebuildMpS */,
            15.0 /* m_finishToleranceM */};
  case VehicleType::Bicycle:
    return {false /* useDirectionForRouteBuilding */,
            true /* m_matchRoute */,
            true /* m_soundDirection */,
            30.0 /* m_matchingThresholdM */,
            false /* m_showTurnAfterNext */,
            -1 /* m_minSpeedForRouteRebuildMpS */,
            15.0 /* m_finishToleranceM */};
  case VehicleType::Car:
    return {true /* useDirectionForRouteBuilding */,
            true /* m_matchRoute */,
            true /* m_soundDirection */,
            50.0 /* m_matchingThresholdM */,
            true /* m_showTurnAfterNext */,
            routing::KMPH2MPS(3.0) /* m_minSpeedForRouteRebuildMpS */,
            20.0 /* m_finishToleranceM */};
  case VehicleType::Count:
    CHECK(false, ("Can't create GetRoutingSettings for", vehicleType));
  }
  UNREACHABLE();
}
}  // namespace routing

