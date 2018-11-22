#include "routing/routing_settings.hpp"

namespace routing
{
// RoutingSettings ---------------------------------------------------------------------------------
RoutingSettings::RoutingSettings(bool matchRoute, bool soundDirection, double matchingThresholdM,
                                 bool keepPedestrianInfo, bool showTurnAfterNext)
  : m_matchRoute(matchRoute)
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
    return {true /* m_matchRoute */,         false /* m_soundDirection */,
            20. /* m_matchingThresholdM */,  true /* m_keepPedestrianInfo */,
            false /* m_showTurnAfterNext */};
  case VehicleType::Transit:
    return {true /* m_matchRoute */,         false /* m_soundDirection */,
            40. /* m_matchingThresholdM */,  true /* m_keepPedestrianInfo */,
            false /* m_showTurnAfterNext */};
  case VehicleType::Bicycle:
    return {true /* m_matchRoute */,         true /* m_soundDirection */,
            30. /* m_matchingThresholdM */,  false /* m_keepPedestrianInfo */,
            false /* m_showTurnAfterNext */};
  case VehicleType::Count:
    CHECK(false, ("Can't create GetRoutingSettings for", vehicleType));
  case VehicleType::Car:
    return {true /* m_matchRoute */,        true /* m_soundDirection */,
            50. /* m_matchingThresholdM */, false /* m_keepPedestrianInfo */,
            true /* m_showTurnAfterNext */};
    // TODO (@gmoryes) make m_speedCameraWarningEnabled to true after tests ok. Now it can be on with:
    // TODO (@gmoryes) typing "?speedcams" in search panel.
  }
  UNREACHABLE();
}
}  // namespace routing
