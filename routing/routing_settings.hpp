#pragma once

#include "std/utility.hpp"

namespace routing
{

/// \brief The RoutingSettings struct is intended to collect all the settings of
/// following along the route.
/// For example, route matching properties, rerouting properties and so on.
struct RoutingSettings
{
  /// \brief if m_matchBearing is equal to true the bearing follows the
  /// route direction if the current position is matched to the route.
  /// If m_matchBearing is equal to false GPS bearing is used while
  /// the current position is matched to the route.
  bool    m_matchBearing;

  /// \brief m_matchingThresholdM is half width of the passage around the route
  /// for route matching in meters. That means if a real current position is closer than
  /// m_matchingThresholdM to the route than the current position is moved to
  /// the closest point to the route.
  double  m_matchingThresholdM;
};

inline RoutingSettings GetPedestrianRoutingSettings()
{
  return RoutingSettings({ false /* m_matchBearing */, 20. /* m_matchingThresholdM */ });
}

inline RoutingSettings GetCarRoutingSettings()
{
  return RoutingSettings({ true /* m_matchBearing */, 50. /* m_matchingThresholdM */ });
}
}  // namespace routing
