#include "routing/edge_estimator.hpp"

#include "traffic/traffic_info.hpp"

#include "std/algorithm.hpp"

using namespace traffic;

namespace
{
double CalcTrafficFactor(SpeedGroup speedGroup)
{
  double constexpr kImpossibleDrivingFactor = 1e4;
  if (speedGroup == SpeedGroup::TempBlock)
    return kImpossibleDrivingFactor;

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
  CHECK_GREATER(speedMPS, 0.0,
                ("from:", MercatorBounds::ToLatLon(from), "to:", MercatorBounds::ToLatLon(to)));

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
  double CalcSegmentWeight(Segment const & segment, RoadGeometry const & road) const override;
  double CalcHeuristic(m2::PointD const & from, m2::PointD const & to) const override;
  double GetUTurnPenalty() const override;

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

double CarEdgeEstimator::CalcSegmentWeight(Segment const & segment, RoadGeometry const & road) const
{
  ASSERT_LESS(segment.GetPointId(true /* front */), road.GetPointsCount(), ());
  ASSERT_LESS(segment.GetPointId(false /* front */), road.GetPointsCount(), ());

  // Current time estimation are too optimistic.
  // Need more accurate tuning: traffic lights, traffic jams, road models and so on.
  // Add some penalty to make estimation of a more realistic.
  // TODO: make accurate tuning, remove penalty.
  double constexpr kTimePenalty = 1.8;

  double const speedMPS = road.GetSpeed() * kKMPH2MPS;
  double result = TimeBetweenSec(road.GetPoint(segment.GetPointId(false /* front */)),
                                 road.GetPoint(segment.GetPointId(true /* front */)), speedMPS) *
                  kTimePenalty;

  if (m_trafficColoring)
  {
    auto const dir = segment.IsForward() ? TrafficInfo::RoadSegmentId::kForwardDirection
                                         : TrafficInfo::RoadSegmentId::kReverseDirection;
    auto const it = m_trafficColoring->find(
        TrafficInfo::RoadSegmentId(segment.GetFeatureId(), segment.GetSegmentIdx(), dir));
    SpeedGroup const speedGroup =
        (it == m_trafficColoring->cend()) ? SpeedGroup::Unknown : it->second;
    ASSERT_LESS(speedGroup, SpeedGroup::Count, ());
    result *= CalcTrafficFactor(speedGroup);
  }

  return result;
}

double CarEdgeEstimator::CalcHeuristic(m2::PointD const & from, m2::PointD const & to) const
{
  return TimeBetweenSec(from, to, m_maxSpeedMPS);
}

double CarEdgeEstimator::GetUTurnPenalty() const
{
  // Adds 2 minutes penalty for U-turn. The value is quite arbitrary
  // and needs to be properly selected after a number of real-world
  // experiments.
  return 2 * 60;  // seconds
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
