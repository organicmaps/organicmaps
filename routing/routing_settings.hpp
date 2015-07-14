#pragma once

#include "std/utility.hpp"

namespace routing
{

/// \brief The RoutingSettings struct is intended to collect all the settings of
/// following along the route.
/// For example, route matching properties, rerouting properties and so on.
struct RoutingSettings
{
  RoutingSettings() : m_matchBearing(true), m_matchingThresholdM(50.) {}
  RoutingSettings(bool routeMatchingBearing, double halfRouteMatchingPassageMeters)
    : m_matchBearing(routeMatchingBearing),
      m_matchingThresholdM(halfRouteMatchingPassageMeters)
  {
  }

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
}  // namespace routing
