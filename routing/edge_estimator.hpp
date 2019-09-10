#pragma once

#include "routing/geometry.hpp"
#include "routing/segment.hpp"
#include "routing/traffic_stash.hpp"
#include "routing/vehicle_mask.hpp"

#include "traffic/traffic_cache.hpp"

#include "routing_common/num_mwm_id.hpp"
#include "routing_common/vehicle_model.hpp"

#include "indexer/mwm_set.hpp"

#include "geometry/point2d.hpp"

#include <memory>

namespace routing
{
class EdgeEstimator
{
public:
  enum class Purpose
  {
    Weight,
    ETA
  };

  EdgeEstimator(double maxWeightSpeedKMpH, SpeedKMpH const & offroadSpeedKMpH);
  virtual ~EdgeEstimator() = default;

  double CalcHeuristic(m2::PointD const & from, m2::PointD const & to) const;
  // Estimates time in seconds it takes to go from point |from| to point |to| along a leap (fake)
  // edge |from|-|to| using real features.
  // Note 1. The result of the method should be used if it's necessary to add a leap (fake) edge
  // (|from|, |to|) in road graph.
  // Note 2. The result of the method should be less or equal to CalcHeuristic(|from|, |to|).
  // Note 3. It's assumed here that CalcLeapWeight(p1, p2) == CalcLeapWeight(p2, p1).
  double CalcLeapWeight(m2::PointD const & from, m2::PointD const & to) const;

  // Estimates time in seconds it takes to go from point |from| to point |to| along direct fake edge.
  double CalcOffroad(m2::PointD const & from, m2::PointD const & to, Purpose purpose) const;

  virtual double CalcSegmentWeight(Segment const & segment, RoadGeometry const & road,
                                   Purpose purpose) const = 0;
  virtual double GetUTurnPenalty(Purpose purpose) const = 0;
  virtual double GetFerryLandingPenalty(Purpose purpose) const = 0;

  static std::shared_ptr<EdgeEstimator> Create(VehicleType vehicleType, double maxWeighSpeedKMpH,
                                               SpeedKMpH const & offroadSpeedKMpH,
                                               std::shared_ptr<TrafficStash>);

  static std::shared_ptr<EdgeEstimator> Create(VehicleType vehicleType,
                                               VehicleModelInterface const & vehicleModel,
                                               std::shared_ptr<TrafficStash>);

private:
  double const m_maxWeightSpeedMpS;
  SpeedKMpH const m_offroadSpeedKMpH;
};
}  // namespace routing
