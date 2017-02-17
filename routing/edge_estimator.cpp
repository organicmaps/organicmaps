#include "routing/edge_estimator.hpp"

#include "traffic/traffic_info.hpp"

#include <algorithm>
#include <unordered_map>

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
  CarEdgeEstimator(shared_ptr<TrafficStash> trafficStash, double maxSpeedKMpH);

  // EdgeEstimator overrides:
  double CalcSegmentWeight(Segment const & segment, RoadGeometry const & road) const override;
  double CalcHeuristic(m2::PointD const & from, m2::PointD const & to) const override;
  double GetUTurnPenalty() const override;
  bool LeapIsAllowed(NumMwmId mwmId) const override;

private:
  shared_ptr<TrafficStash> m_trafficStash;
  double const m_maxSpeedMPS;
};

CarEdgeEstimator::CarEdgeEstimator(shared_ptr<TrafficStash> trafficStash, double maxSpeedKMpH)
  : m_trafficStash(trafficStash), m_maxSpeedMPS(maxSpeedKMpH * kKMPH2MPS)
{
  CHECK_GREATER(m_maxSpeedMPS, 0.0, ());
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

  auto const * trafficColoring = m_trafficStash->Get(segment.GetMwmId());
  if (trafficColoring)
  {
    auto const dir = segment.IsForward() ? TrafficInfo::RoadSegmentId::kForwardDirection
                                         : TrafficInfo::RoadSegmentId::kReverseDirection;
    auto const it = trafficColoring->find(
        TrafficInfo::RoadSegmentId(segment.GetFeatureId(), segment.GetSegmentIdx(), dir));
    SpeedGroup const speedGroup =
        (it == trafficColoring->cend()) ? SpeedGroup::Unknown : it->second;
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

bool CarEdgeEstimator::LeapIsAllowed(NumMwmId mwmId) const { return !m_trafficStash->Has(mwmId); }
}  // namespace

namespace routing
{
// static
shared_ptr<EdgeEstimator> EdgeEstimator::CreateForCar(shared_ptr<TrafficStash> trafficStash,
                                                      double maxSpeedKMpH)
{
  return make_shared<CarEdgeEstimator>(trafficStash, maxSpeedKMpH);
}
}  // namespace routing
