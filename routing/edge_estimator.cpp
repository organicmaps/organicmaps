#include "routing/edge_estimator.hpp"

#include "traffic/traffic_info.hpp"

#include "std/algorithm.hpp"

using namespace traffic;

namespace
{
double CalcTrafficFactor(SpeedGroup speedGroup)
{
  double const percentage =
      0.01 * static_cast<double>(kSpeedGroupThresholdPercentage[static_cast<size_t>(speedGroup)]);
  CHECK_GREATER(percentage, 0.0, ("Speed group:", speedGroup));
  return 1.0 / percentage;
}
}  // namespace

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
  CarEdgeEstimator(IVehicleModel const & vehicleModel, traffic::TrafficCache const & trafficCache);

  // EdgeEstimator overrides:
  void Start(MwmSet::MwmId const & mwmId) override;
  void Finish() override;
  double CalcEdgesWeight(uint32_t featureId, RoadGeometry const & road, uint32_t pointFrom,
                         uint32_t pointTo) const override;
  double CalcHeuristic(m2::PointD const & from, m2::PointD const & to) const override;

private:
  TrafficCache const & m_trafficCache;
  shared_ptr<traffic::TrafficInfo::Coloring> m_trafficColoring;
  double const m_maxSpeedMPS;
};

CarEdgeEstimator::CarEdgeEstimator(IVehicleModel const & vehicleModel,
                                   traffic::TrafficCache const & trafficCache)
  : m_trafficCache(trafficCache), m_maxSpeedMPS(vehicleModel.GetMaxSpeed() * kKMPH2MPS)
{
}

void CarEdgeEstimator::Start(MwmSet::MwmId const & mwmId)
{
  m_trafficColoring = m_trafficCache.GetTrafficInfo(mwmId);
}

void CarEdgeEstimator::Finish()
{
  m_trafficColoring.reset();
}

double CarEdgeEstimator::CalcEdgesWeight(uint32_t featureId, RoadGeometry const & road,
                                         uint32_t pointFrom, uint32_t pointTo) const
{
  uint32_t const start = min(pointFrom, pointTo);
  uint32_t const finish = max(pointFrom, pointTo);
  ASSERT_LESS(finish, road.GetPointsCount(), ());

  // Current time estimation are too optimistic.
  // Need more accurate tuning: traffic lights, traffic jams, road models and so on.
  // Add some penalty to make estimation of a more realistic.
  // TODO: make accurate tuning, remove penalty.
  double constexpr kTimePenalty = 1.8;

  double result = 0.0;
  double const speedMPS = road.GetSpeed() * kKMPH2MPS;
  auto const dir = pointFrom < pointTo ? TrafficInfo::RoadSegmentId::kForwardDirection
                                       : TrafficInfo::RoadSegmentId::kReverseDirection;
  for (uint32_t i = start; i < finish; ++i)
  {
    double edgeWeight =
        TimeBetweenSec(road.GetPoint(i), road.GetPoint(i + 1), speedMPS) * kTimePenalty;
    if (m_trafficColoring)
    {
      auto const it = m_trafficColoring->find(TrafficInfo::RoadSegmentId(featureId, i, dir));
      SpeedGroup const speedGroup = (it == m_trafficColoring->cend()) ? SpeedGroup::Unknown
                                                                      : it->second;
      ASSERT_LESS(speedGroup, SpeedGroup::Count, ());
      edgeWeight *= CalcTrafficFactor(speedGroup);
    }
    result += edgeWeight;
  }

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
shared_ptr<EdgeEstimator> EdgeEstimator::CreateForCar(IVehicleModel const & vehicleModel,
                                                      traffic::TrafficCache const & trafficCache)
{
  return make_shared<CarEdgeEstimator>(vehicleModel, trafficCache);
}
}  // namespace routing
