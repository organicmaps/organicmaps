#include "routing/edge_estimator.hpp"

#include "traffic/traffic_info.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <unordered_map>

using namespace std;
using namespace traffic;

namespace
{
double constexpr kKMPH2MPS = 1000.0 / (60 * 60);

double TimeBetweenSec(m2::PointD const & from, m2::PointD const & to, double speedMPS)
{
  CHECK_GREATER(speedMPS, 0.0,
                ("from:", MercatorBounds::ToLatLon(from), "to:", MercatorBounds::ToLatLon(to)));

  double const distanceM = MercatorBounds::DistanceOnEarth(from, to);
  return distanceM / speedMPS;
}

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
// EdgeEstimator -----------------------------------------------------------------------------------
EdgeEstimator::EdgeEstimator(double maxSpeedKMpH) : m_maxSpeedMPS(maxSpeedKMpH * kKMPH2MPS)
{
  CHECK_GREATER(m_maxSpeedMPS, 0.0, ());
}

double EdgeEstimator::CalcSegmentWeight(Segment const & segment, RoadGeometry const & road) const
{
  ASSERT_LESS(segment.GetPointId(true /* front */), road.GetPointsCount(), ());
  ASSERT_LESS(segment.GetPointId(false /* front */), road.GetPointsCount(), ());

  double const speedMPS = road.GetSpeed() * kKMPH2MPS;
  double result = TimeBetweenSec(road.GetPoint(segment.GetPointId(false /* front */)),
                                 road.GetPoint(segment.GetPointId(true /* front */)), speedMPS);

  return result;
}

double EdgeEstimator::CalcHeuristic(m2::PointD const & from, m2::PointD const & to) const
{
  return TimeBetweenSec(from, to, m_maxSpeedMPS);
}

double EdgeEstimator::CalcLeapWeight(m2::PointD const & from, m2::PointD const & to) const
{
  // Let us assume for the time being that
  // leap edges will be added with a half of max speed.
  // @TODO(bykoianko) It's necessary to gather statistics to calculate better factor(s) instead of
  // one below.
  return TimeBetweenSec(from, to, m_maxSpeedMPS / 2.0);
}

// PedestrianEstimator -----------------------------------------------------------------------------
class PedestrianEstimator final : public EdgeEstimator
{
public:
  explicit PedestrianEstimator(double maxSpeedKMpH) : EdgeEstimator(maxSpeedKMpH) {}

  // EdgeEstimator overrides:
  double GetUTurnPenalty() const override { return 0.0 /* seconds */; }
  bool LeapIsAllowed(NumMwmId /* mwmId */) const override { return false; }
};

// BicycleEstimator --------------------------------------------------------------------------------
class BicycleEstimator final : public EdgeEstimator
{
public:
  explicit BicycleEstimator(double maxSpeedKMpH) : EdgeEstimator(maxSpeedKMpH) {}

  // EdgeEstimator overrides:
  double GetUTurnPenalty() const override { return 20.0 /* seconds */; }
  bool LeapIsAllowed(NumMwmId /* mwmId */) const override { return false; }
};

// CarEstimator ------------------------------------------------------------------------------------
class CarEstimator final : public EdgeEstimator
{
public:
  CarEstimator(shared_ptr<TrafficStash> trafficStash, double maxSpeedKMpH);

  // EdgeEstimator overrides:
  double CalcSegmentWeight(Segment const & segment, RoadGeometry const & road) const override;
  double GetUTurnPenalty() const override;
  bool LeapIsAllowed(NumMwmId mwmId) const override;

private:
  shared_ptr<TrafficStash> m_trafficStash;
};

CarEstimator::CarEstimator(shared_ptr<TrafficStash> trafficStash, double maxSpeedKMpH)
  : EdgeEstimator(maxSpeedKMpH)
  , m_trafficStash(move(trafficStash))
{
}

double CarEstimator::CalcSegmentWeight(Segment const & segment, RoadGeometry const & road) const
{
  // Current time estimation are too optimistic.
  // Need more accurate tuning: traffic lights, traffic jams, road models and so on.
  // Add some penalty to make estimation of a more realistic.
  // TODO: make accurate tuning, remove penalty.
  double constexpr kTimePenalty = 1.8;

  double result = EdgeEstimator::CalcSegmentWeight(segment, road);

  if (m_trafficStash)
  {
    SpeedGroup const speedGroup = m_trafficStash->GetSpeedGroup(segment);
    ASSERT_LESS(speedGroup, SpeedGroup::Count, ());
    double const trafficFactor = CalcTrafficFactor(speedGroup);
    result *= trafficFactor;
    if (speedGroup != SpeedGroup::Unknown && speedGroup != SpeedGroup::G5)
      result *= kTimePenalty;
  }

  return result;
}

double CarEstimator::GetUTurnPenalty() const
{
  // Adds 2 minutes penalty for U-turn. The value is quite arbitrary
  // and needs to be properly selected after a number of real-world
  // experiments.
  return 2 * 60;  // seconds
}

bool CarEstimator::LeapIsAllowed(NumMwmId mwmId) const { return !m_trafficStash->Has(mwmId); }

// EdgeEstimator -----------------------------------------------------------------------------------
// static
shared_ptr<EdgeEstimator> EdgeEstimator::Create(VehicleType vehicleType, double maxSpeedKMpH,
                                                shared_ptr<TrafficStash> trafficStash)
{
  switch (vehicleType)
  {
  case VehicleType::Pedestrian: return make_shared<PedestrianEstimator>(maxSpeedKMpH);
  case VehicleType::Bicycle: return make_shared<BicycleEstimator>(maxSpeedKMpH);
  case VehicleType::Car: return make_shared<CarEstimator>(trafficStash, maxSpeedKMpH);
  case VehicleType::Count:
    CHECK(false, ("Can't create EdgeEstimator for", vehicleType));
    return nullptr;
  }
}
}  // namespace routing
