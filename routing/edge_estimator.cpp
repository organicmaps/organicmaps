#include "routing/edge_estimator.hpp"

#include "std/algorithm.hpp"

namespace routing
{
double constexpr kKMPH2MPS = 1000.0 / (60 * 60);

inline double TimeBetweenSec(m2::PointD const & from, m2::PointD const & to, double speedMPS)
{
  ASSERT_GREATER(speedMPS, 0.0, ());

  double const distanceM = MercatorBounds::DistanceOnEarth(from, to);
  return distanceM / speedMPS;
}

class CarEdgeEstimator : public EdgeEstimator
{
public:
  CarEdgeEstimator(IVehicleModel const & vehicleModel);

  // EdgeEstimator overrides:
  double CalcEdgesWeight(RoadGeometry const & road, uint32_t pointFrom,
                         uint32_t pointTo) const override;
  double CalcHeuristic(m2::PointD const & from, m2::PointD const & to) const override;

private:
  double const m_maxSpeedMPS;
};

CarEdgeEstimator::CarEdgeEstimator(IVehicleModel const & vehicleModel)
  : m_maxSpeedMPS(vehicleModel.GetMaxSpeed() * kKMPH2MPS)
{
}

double CarEdgeEstimator::CalcEdgesWeight(RoadGeometry const & road, uint32_t pointFrom,
                                         uint32_t pointTo) const
{
  uint32_t const start = min(pointFrom, pointTo);
  uint32_t const finish = max(pointFrom, pointTo);
  ASSERT_LESS(finish, road.GetPointsCount(), ());

  double result = 0.0;
  double const speedMPS = road.GetSpeed() * kKMPH2MPS;
  for (uint32_t i = start; i < finish; ++i)
    result += TimeBetweenSec(road.GetPoint(i), road.GetPoint(i + 1), speedMPS);

  return result;
}

double CarEdgeEstimator::CalcHeuristic(m2::PointD const & from, m2::PointD const & to) const
{
  return TimeBetweenSec(from, to, m_maxSpeedMPS);
}
}  // namespace

namespace routing
{
// static
shared_ptr<EdgeEstimator> EdgeEstimator::CreateForCar(IVehicleModel const & vehicleModel)
{
  return make_shared<CarEdgeEstimator>(vehicleModel);
}
}  // namespace routing
