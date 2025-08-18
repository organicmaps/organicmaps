#pragma once

#include "routing/vehicle_mask.hpp"

namespace routing
{
/// \brief The RoutingSettings struct is intended to collect all the settings of
/// following along the route.
/// For example, route matching properties, rerouting properties and so on.
struct RoutingSettings
{
  friend RoutingSettings GetRoutingSettings(VehicleType vehicleType);

private:
  RoutingSettings(bool useDirectionForRouteBuilding, bool matchRoute, bool soundDirection, double matchingThresholdM,
                  bool showTurnAfterNext, double minSpeedForRouteRebuildMpS, double finishToleranceM,
                  size_t maxOutgoingPointsCount, double minOutgoingDistMeters, size_t maxIngoingPointsCount,
                  double minIngoingDistMeters, size_t notSoCloseMaxPointsCount, double notSoCloseMaxDistMeters);

public:
  /// \brief We accumulate several positions to calculate current direction.
  /// So we can use this direction for car for ex. or don't for pedestrian for ex.
  bool m_useDirectionForRouteBuilding;
  /// \brief if m_matchRoute is equal to true the bearing follows the
  /// route direction if the current position is matched to the route.
  /// If m_matchRoute is equal to false GPS bearing is used while
  /// the current position is matched to the route.
  bool m_matchRoute;
  /// \brief if m_soundDirection is equal to true an end user gets sound notification
  /// before directions.
  bool m_soundDirection;

  /// \brief m_matchingThresholdM is half width of the passage around the route
  /// for route matching in meters. That means if a real current position is closer than
  /// m_matchingThresholdM to the route than the current position is moved to
  /// the closest point to the route.
  double m_matchingThresholdM;

  /// \brief if m_showTurnAfterNext is equal to true end users see a notification
  /// about the turn after the next in some cases.
  bool m_showTurnAfterNext;

  /// \brief If the speed in gps info is less than |m_minSpeedForRouteRebuildMpS|
  /// info we will penalize less than if it's would be.
  /// Penalty uses for route rebuilding during routing.
  double m_minSpeedForRouteRebuildMpS;

  /// \brief The distance to the route finish point at which the route should be completed.
  double m_finishToleranceM;

  // These 4 parameters are used during turns generation to calculate the turn angle. Based on these
  // parameters, the point before the turn and the point after the turn are determined, and the
  // angle between them is calculated.
  size_t m_maxOutgoingPointsCount;
  double m_minOutgoingDistMeters;
  size_t m_maxIngoingPointsCount;
  double m_minIngoingDistMeters;
  /// @todo These 2 parameters are not used anymore.
  /// It should me removed. But desktop crashes after removal.
  size_t m_notSoCloseMaxPointsCount;
  double m_notSoCloseMaxDistMeters;
};

RoutingSettings GetRoutingSettings(VehicleType vehicleType);
}  // namespace routing
