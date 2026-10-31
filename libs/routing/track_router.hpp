#pragma once

#include "routing/router.hpp"
#include "routing/vehicle_mask.hpp"

#include "geometry/point_with_altitude.hpp"

#include <memory>
#include <string>
#include <vector>

namespace routing
{
// One immutable connected polyline and caller-supplied ordered stop indexes.
struct TrackFollowingData
{
  std::vector<geometry::PointWithAltitude> m_points;
  // Non-decreasing indexes into m_points; duplicate indexes represent collocated stops.
  std::vector<size_t> m_stopPointIndexes;
};

// Builds the remainder of one connected polyline from the nearest projection of the start.
// Checkpoints must correspond to the supplied stop indexes and the track finish. Projection
// ties use the earliest occurrence; live progress and source-occurrence selection belong to
// the caller. Every checkpoint leg owns a subroute, including empty passed legs.
class TrackRouter : public IRouter
{
public:
  // Data must contain at least two points and remain immutable for the lifetime of the router.
  TrackRouter(VehicleType vehicleType, std::shared_ptr<TrackFollowingData const> data);

  // IRouter overrides:
  std::string GetName() const override { return "track-router"; }
  void ClearState() override {}
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
  struct Projection
  {
    size_t m_segIdx = 0;      // Index of the track segment [m_segIdx, m_segIdx + 1].
    double m_fraction = 0.0;  // Position on the segment, in [0, 1].
  };

  // Junctions and cumulative distances, with one leg end per unpassed checkpoint.
  struct RouteGeometry
  {
    std::vector<geometry::PointWithAltitude> m_junctions;
    std::vector<double> m_distM;
    std::vector<size_t> m_legEndJunctions;
    // The first on-track junction (the projection point); m_distM[m_projJunctionIdx] is the
    // length of the lead-in onto the track.
    size_t m_projJunctionIdx = 0;
  };

  // Nearest point on the supplied connected polyline; live progress belongs to the caller.
  Projection ProjectPoint(m2::PointD const & point, RouterDelegate const & delegate) const;
  RouteGeometry AssembleRouteGeometry(Checkpoints const & checkpoints, Projection const & proj,
                                      RouterDelegate const & delegate) const;

  bool const m_pedestrian;
  double const m_speedMpS;
  std::shared_ptr<TrackFollowingData const> const m_data;
};
}  // namespace routing
