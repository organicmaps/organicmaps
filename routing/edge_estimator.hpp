#pragma once

#include "routing/geometry.hpp"
#include "routing/segment.hpp"
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

  virtual void Start(MwmSet::MwmId const & mwmId) = 0;
  virtual void Finish() = 0;
  virtual double CalcSegmentWeight(Segment const & segment, RoadGeometry const & road) const = 0;
  virtual double CalcHeuristic(m2::PointD const & from, m2::PointD const & to) const = 0;
  virtual double GetUTurnPenalty() const = 0;

  static shared_ptr<EdgeEstimator> CreateForCar(IVehicleModel const & vehicleModel,
                                                traffic::TrafficCache const & trafficCache);
};

class EstimatorGuard final
{
public:
  EstimatorGuard(MwmSet::MwmId const & mwmId, EdgeEstimator & estimator) : m_estimator(estimator)
  {
    m_estimator.Start(mwmId);
  }

  ~EstimatorGuard() { m_estimator.Finish(); }

private:
  EdgeEstimator & m_estimator;
};
}  // namespace routing
