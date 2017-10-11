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
  explicit EdgeEstimator(double maxSpeedKMpH);
  virtual ~EdgeEstimator() = default;

  double CalcHeuristic(m2::PointD const & from, m2::PointD const & to) const;
  // Returns time in seconds it takes to go from point |from| to point |to| along a leap (fake)
  // edge |from|-|to|.
  // Note 1. The result of the method should be used if it's necessary to add a leap (fake) edge
  // (|from|, |to|) in road graph.
  // Note 2. The result of the method should be less or equal to CalcHeuristic(|from|, |to|).
  // Note 3. It's assumed here that CalcLeapWeight(p1, p2) == CalcLeapWeight(p2, p1).
  double CalcLeapWeight(m2::PointD const & from, m2::PointD const & to) const;

  virtual double CalcSegmentWeight(Segment const & segment, RoadGeometry const & road) const = 0;
  virtual double GetUTurnPenalty() const = 0;
  // The leap is the shortcut edge from mwm border enter to exit.
  // Router can't use leaps on some mwms: e.g. mwm with loaded traffic data.
  // Check wherether leap is allowed on specified mwm or not.
  virtual bool LeapIsAllowed(NumMwmId mwmId) const = 0;

  static std::shared_ptr<EdgeEstimator> Create(VehicleType, double maxSpeedKMpH,
                                               std::shared_ptr<TrafficStash>);

private:
  double const m_maxSpeedMPS;
};
}  // namespace routing
