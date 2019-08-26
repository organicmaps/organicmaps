#include "routing/routing_settings.hpp"

#include "routing/routing_helpers.hpp"

namespace routing
{
// RoutingSettings ---------------------------------------------------------------------------------
RoutingSettings::RoutingSettings(bool useDirectionForRouteBuilding, bool matchRoute, bool soundDirection,
                                 double matchingThresholdM, bool keepPedestrianInfo,
                                 bool showTurnAfterNext, double minSpeedForRouteRebuildMpS)
  : m_useDirectionForRouteBuilding(useDirectionForRouteBuilding)
  , m_matchRoute(matchRoute)
  , m_soundDirection(soundDirection)
  , m_matchingThresholdM(matchingThresholdM)
  , m_keepPedestrianInfo(keepPedestrianInfo)
  , m_showTurnAfterNext(showTurnAfterNext)
{
}

RoutingSettings GetRoutingSettings(VehicleType vehicleType)
{
  switch (vehicleType)
  {
  case VehicleType::Pedestrian:
    return {false /* useDirectionForRouteBuilding */,
            true /* m_matchRoute */,         false /* m_soundDirection */,
            20. /* m_matchingThresholdM */,  true /* m_keepPedestrianInfo */,
            false /* m_showTurnAfterNext */, -1 /* m_minSpeedForRouteRebuildMpS */};
  case VehicleType::Transit:
    return {false /* useDirectionForRouteBuilding */,
            true /* m_matchRoute */,         false /* m_soundDirection */,
            40. /* m_matchingThresholdM */,  true /* m_keepPedestrianInfo */,
            false /* m_showTurnAfterNext */, -1 /* m_minSpeedForRouteRebuildMpS */};
  case VehicleType::Bicycle:
    return {false /* useDirectionForRouteBuilding */,
            true /* m_matchRoute */,         true /* m_soundDirection */,
            30. /* m_matchingThresholdM */,  false /* m_keepPedestrianInfo */,
            false /* m_showTurnAfterNext */, -1 /* m_minSpeedForRouteRebuildMpS */};
  case VehicleType::Car:
    return {true /* useDirectionForRouteBuilding */,
            true /* m_matchRoute */,        true /* m_soundDirection */,
            50. /* m_matchingThresholdM */, false /* m_keepPedestrianInfo */,
            true /* m_showTurnAfterNext */, routing::KMPH2MPS(3) /* m_minSpeedForRouteRebuildMpS */};
  case VehicleType::Count:
    CHECK(false, ("Can't create GetRoutingSettings for", vehicleType));
  }
  UNREACHABLE();
}
}  // namespace routing

