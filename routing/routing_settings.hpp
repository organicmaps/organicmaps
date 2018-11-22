#pragma once

#include "routing/vehicle_mask.hpp"

#include "base/assert.hpp"

namespace routing
{

/// \brief The RoutingSettings struct is intended to collect all the settings of
/// following along the route.
/// For example, route matching properties, rerouting properties and so on.
struct RoutingSettings
{
  friend RoutingSettings GetRoutingSettings(VehicleType vehicleType);

private:
  RoutingSettings(bool matchRoute, bool soundDirection, double matchingThresholdM,
                  bool keepPedestrianInfo, bool showTurnAfterNext);

public:
  /// \brief if m_matchRoute is equal to true the bearing follows the
  /// route direction if the current position is matched to the route.
  /// If m_matchRoute is equal to false GPS bearing is used while
  /// the current position is matched to the route.
  bool    m_matchRoute;
  /// \brief if m_soundDirection is equal to true an end user gets sound notification
  /// before directions.
  bool    m_soundDirection;

  /// \brief m_matchingThresholdM is half width of the passage around the route
  /// for route matching in meters. That means if a real current position is closer than
  /// m_matchingThresholdM to the route than the current position is moved to
  /// the closest point to the route.
  double  m_matchingThresholdM;

  /// \brief m_keepPedestrianInfo flag for keeping in memory additional information for pedestrian
  /// routing.
  bool m_keepPedestrianInfo;

  /// \brief if m_showTurnAfterNext is equal to true end users see a notification
  /// about the turn after the next in some cases.
  bool m_showTurnAfterNext;
};

RoutingSettings GetRoutingSettings(VehicleType vehicleType);
}  // namespace routing
