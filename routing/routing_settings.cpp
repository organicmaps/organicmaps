#include "routing/routing_settings.hpp"

namespace routing
{
RoutingSettings GetRoutingSettings(VehicleType vehicleType)
{
  switch (vehicleType)
  {
  case VehicleType::Pedestrian:
  case VehicleType::Transit:
    return {true /* m_matchRoute */,         false /* m_soundDirection */,
            20. /* m_matchingThresholdM */,  true /* m_keepPedestrianInfo */,
            false /* m_showTurnAfterNext */, false /* m_speedCameraWarning*/};
  case VehicleType::Bicycle:
    return {true /* m_matchRoute */,         true /* m_soundDirection */,
            30. /* m_matchingThresholdM */,  false /* m_keepPedestrianInfo */,
            false /* m_showTurnAfterNext */, false /* m_speedCameraWarning*/};
  case VehicleType::Car:
    return {true /* m_matchRoute */,        true /* m_soundDirection */,
            50. /* m_matchingThresholdM */, false /* m_keepPedestrianInfo */,
            true /* m_showTurnAfterNext */, true /* m_speedCameraWarning*/};
  case VehicleType::Count:
    CHECK(false, ("Can't create GetRoutingSettings for", vehicleType));
    return {};
  }
}
}  // namespace routing
