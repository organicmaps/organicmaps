#pragma once

#include "routing/router.hpp"
#include "routing/vehicle_mask.hpp"

#include "kml/type_utils.hpp"

#include <memory>
#include <string>
#include <vector>

namespace routing
{
// Immutable connected remainder in traversal order, starting at the caller-selected entry.
struct TrackFollowingData
{
  kml::TrackGeometry m_points;
  // Non-decreasing indexes into m_points; duplicate indexes represent collocated stops.
  std::vector<size_t> m_stopPointIndexes;
};

// Builds the supplied remainder with a straight approach from the start to its first point.
// The caller owns entry selection, direction and live progress. Checkpoints must be fresh
// and correspond to the supplied stop indexes and the track finish. Every checkpoint leg
// owns a subroute.
class TrackRouter : public IRouter
{
public:
  // Supports car, bicycle and pedestrian profiles.
  // Data must contain at least two points and remain immutable for the lifetime of the router.
  TrackRouter(VehicleType vehicleType, std::shared_ptr<TrackFollowingData const> data);

  // IRouter overrides:
  std::string GetName() const override { return "track-router"; }
  void ClearState() override {}
  bool IsTrafficSensitive() const override { return false; }
  void SetGuides(GuidesTracks && /* guides */) override {}
  RouterResultCode CalculateRoute(Checkpoints const & checkpoints, m2::PointD const & startDirection,
                                  bool adjustToPrevRoute, bool needAlternatives, RouterDelegate const & delegate,
                                  RoutesResult & result) override;
  bool FindClosestProjectionToRoad(m2::PointD const & point, m2::PointD const & direction, double radius,
                                   EdgeProj & proj) override
  {
    // Track router has no connection to the road graph.
    return false;
  }

private:
  std::shared_ptr<TrackFollowingData const> const m_data;
  VehicleType const m_vehicleType;
};
}  // namespace routing
