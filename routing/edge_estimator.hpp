#pragma once

#include "routing/geometry.hpp"
#include "routing/num_mwm_id.hpp"
#include "routing/segment.hpp"
#include "routing/traffic_stash.hpp"
#include "routing/vehicle_model.hpp"

#include "traffic/traffic_cache.hpp"

#include "indexer/mwm_set.hpp"

#include "geometry/point2d.hpp"

#include "std/cstdint.hpp"
#include "std/shared_ptr.hpp"

namespace routing
{
class EdgeEstimator
{
public:
  virtual ~EdgeEstimator() = default;

  virtual double CalcSegmentWeight(Segment const & segment, RoadGeometry const & road) const = 0;
  virtual double CalcHeuristic(m2::PointD const & from, m2::PointD const & to) const = 0;
  virtual double GetUTurnPenalty() const = 0;

  static shared_ptr<EdgeEstimator> CreateForCar(shared_ptr<TrafficStash> trafficStash,
                                                double maxSpeedKMpH);
};
}  // namespace routing
