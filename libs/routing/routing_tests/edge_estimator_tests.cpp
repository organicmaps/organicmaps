#include "testing/testing.hpp"

#include "routing/edge_estimator.hpp"
#include "routing/geometry.hpp"
#include "routing/segment.hpp"

#include "routing_common/maxspeed_conversion.hpp"
#include "routing_common/vehicle_model.hpp"

#include "platform/measurement_utils.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point_with_altitude.hpp"

#include <cmath>

namespace
{
using namespace routing;

double const kTan = 0.1;
geometry::Altitude const kAlt = 100.0;
auto const kPurposes = {EdgeEstimator::Purpose::Weight, EdgeEstimator::Purpose::ETA};
constexpr double kAccuracyEps = 1e-5;

// Climb penalty on plain surface (tangent = 0) must be 1.0 for ETA and Weight estimations.
UNIT_TEST(ClimbPenalty_ZeroTangent)
{
  double const zeroTangent = 0.0;

  for (auto const & purpose : kPurposes)
  {
    TEST_EQUAL(GetCarClimbPenalty(purpose, zeroTangent, kAlt), 1.0, ());
    TEST_EQUAL(GetBicycleClimbPenalty(purpose, zeroTangent, kAlt), 1.0, ());
    TEST_EQUAL(GetPedestrianClimbPenalty(purpose, zeroTangent, kAlt), 1.0, ());
  }
}

// Descent penalty for pedestrians and bicycles must be less then the ascent penalty.
UNIT_TEST(ClimbPenalty_DescentLessThenAscent)
{
  for (auto const & purpose : kPurposes)
  {
    double const ascPenaltyPedestrian = GetPedestrianClimbPenalty(purpose, kTan, kAlt);
    double const descPenaltyPedestrian = GetPedestrianClimbPenalty(purpose, -kTan, kAlt);
    TEST_LESS(descPenaltyPedestrian, ascPenaltyPedestrian, ());

    double const ascPenaltyBicycle = GetBicycleClimbPenalty(purpose, kTan, kAlt);
    double const descPenaltyBicycle = GetBicycleClimbPenalty(purpose, -kTan, kAlt);
    TEST_LESS(descPenaltyBicycle, ascPenaltyBicycle, ());
  }
}

// Descent penalty for cars must be equal to the ascent penalty.
UNIT_TEST(ClimbPenalty_DescentEqualsAscent)
{
  for (auto const & purpose : kPurposes)
  {
    double const ascPenaltyCar = GetCarClimbPenalty(purpose, kTan, kAlt);
    double const descPenaltyCar = GetCarClimbPenalty(purpose, -kTan, kAlt);
    TEST_EQUAL(ascPenaltyCar, 1.0, ());
    TEST_EQUAL(descPenaltyCar, 1.0, ());
  }
}

// Climb penalty high above the sea level (higher then 2.5 km) should be very significant.
UNIT_TEST(ClimbPenalty_HighAboveSeaLevel)
{
  for (auto const & purpose : kPurposes)
  {
    double const penalty2500 = GetPedestrianClimbPenalty(purpose, kTan, 2500);
    double const penalty4000 = GetPedestrianClimbPenalty(purpose, kTan, 4000);
    double const penalty5500 = GetPedestrianClimbPenalty(purpose, kTan, 5500);
    double const penalty7000 = GetPedestrianClimbPenalty(purpose, kTan, 7000);

    TEST_GREATER_OR_EQUAL(penalty2500, 2.0, ());
    TEST_GREATER_OR_EQUAL(penalty4000, penalty2500 + 1.0, ());
    TEST_GREATER_OR_EQUAL(penalty5500, penalty4000 + 1.0, ());
    TEST_GREATER_OR_EQUAL(penalty7000, penalty5500 + 1.0, ());

    double const penalty2500Bicyclce = GetBicycleClimbPenalty(purpose, kTan, 2500);

    TEST_GREATER_OR_EQUAL(penalty2500Bicyclce, 6.0, ());
    TEST_ALMOST_EQUAL_ABS(GetCarClimbPenalty(purpose, kTan, 2500), 1.0, kAccuracyEps, ());
  }
}

// Distance-biased strategy: roads at/above the per-vehicle cap speed tie by pure distance, slower
// roads keep their Normal weight, the weight never drops below distance / maxWeightSpeed (A*
// heuristic admissibility) and ETA does not depend on the strategy.
UNIT_TEST(DistanceBiasedStrategy_CapsFastRoadsKeepsSlowRoads)
{
  auto const makeRoad = [](uint16_t speedKmPH)
  {
    return RoadGeometry(true /* oneWay */, Maxspeed(measurement_utils::Units::Metric, speedKmPH, kInvalidSpeed),
                        RoadGeometry::Points({{0.0, 0.0}, {0.0, 0.01}}));
  };

  struct VehicleCase
  {
    VehicleType m_type;
    double m_maxWeightSpeedKMpH;
    SpeedKMpH m_offroad;
    // Both fast speeds are above the vehicle's distance-bias cap, the slow one is below.
    uint16_t m_fast1, m_fast2, m_slow;
  };
  std::initializer_list<VehicleCase> const cases = {
      {VehicleType::Car, 200.0, {0.01 /* weight */, kNotUsed /* eta */}, 120, 80, 5},
      {VehicleType::Bicycle, 100.0, {1.5, 3.0}, 25, 18, 2},
      {VehicleType::Pedestrian, 60.0, {0.5, 3.0}, 7, 6, 3},
  };

  Segment const segment(0 /* mwmId */, 0 /* featureId */, 0 /* segmentIdx */, true /* forward */);
  for (auto const & c : cases)
  {
    auto const estimator =
        EdgeEstimator::Create(c.m_type, c.m_maxWeightSpeedKMpH, c.m_offroad, nullptr /* trafficStash */,
                              nullptr /* dataSourcePtr */, nullptr /* numMwmIds */);
    auto const roadFast1 = makeRoad(c.m_fast1);
    auto const roadFast2 = makeRoad(c.m_fast2);
    auto const roadSlow = makeRoad(c.m_slow);
    double const distanceM = roadFast1.GetDistance(0);

    auto const calc = [&](RoadGeometry const & road, EdgeEstimator::Purpose purpose)
    { return estimator->CalcSegmentWeight(segment, road, purpose, 0 /* arrivalTime */); };

    double const etaFast = calc(roadFast1, EdgeEstimator::Purpose::ETA);
    double const normalFast1 = calc(roadFast1, EdgeEstimator::Purpose::Weight);
    double const normalFast2 = calc(roadFast2, EdgeEstimator::Purpose::Weight);
    double const normalSlow = calc(roadSlow, EdgeEstimator::Purpose::Weight);
    TEST_NOT_EQUAL(normalFast1, normalFast2, (c.m_type));

    estimator->SetStrategy(EdgeEstimator::Strategy::DistanceBiased);
    double const distanceBiasedFast1 = calc(roadFast1, EdgeEstimator::Purpose::Weight);
    double const distanceBiasedFast2 = calc(roadFast2, EdgeEstimator::Purpose::Weight);
    double const distanceBiasedSlow = calc(roadSlow, EdgeEstimator::Purpose::Weight);

    // Above-cap roads are ranked by pure distance: equal weights, higher than their Normal ones.
    TEST_ALMOST_EQUAL_ABS(distanceBiasedFast1, distanceBiasedFast2, kAccuracyEps, (c.m_type));
    TEST_GREATER(distanceBiasedFast1, normalFast1, (c.m_type));
    // A below-cap road keeps its Normal weight and stays more expensive than any capped road.
    TEST_ALMOST_EQUAL_ABS(distanceBiasedSlow, normalSlow, kAccuracyEps, (c.m_type));
    TEST_GREATER(distanceBiasedSlow, distanceBiasedFast1, (c.m_type));
    // Weights never drop below the A* heuristic bound.
    TEST_GREATER_OR_EQUAL(distanceBiasedFast1, distanceM / measurement_utils::KmphToMps(c.m_maxWeightSpeedKMpH),
                          (c.m_type));
    // ETA is strategy-independent.
    TEST_ALMOST_EQUAL_ABS(calc(roadFast1, EdgeEstimator::Purpose::ETA), etaFast, kAccuracyEps, (c.m_type));

    // The tight heuristic engages only under DistanceBiased with |tightAllowed|: it grows, but
    // exactly to the cheapest capped road weight over the same distance (stays admissible).
    ms::LatLon const fromLL = mercator::ToLatLon(m2::PointD(0.0, 0.0));
    ms::LatLon const toLL = mercator::ToLatLon(m2::PointD(0.0, 0.01));
    double const looseHeuristic = estimator->CalcHeuristic(fromLL, toLL);
    double const tightHeuristic = estimator->CalcHeuristic(fromLL, toLL, true /* tightAllowed */);
    TEST_GREATER_OR_EQUAL(tightHeuristic, looseHeuristic, (c.m_type));
    TEST_ALMOST_EQUAL_ABS(tightHeuristic, distanceBiasedFast1, kAccuracyEps, (c.m_type));

    // Guides gate: tightHeuristicAllowed == false keeps the loose bound even under DistanceBiased.
    estimator->SetStrategy(EdgeEstimator::Strategy::DistanceBiased, false /* tightHeuristicAllowed */);
    TEST_ALMOST_EQUAL_ABS(estimator->CalcHeuristic(fromLL, toLL, true /* tightAllowed */), looseHeuristic, kAccuracyEps,
                          (c.m_type));

    estimator->SetStrategy(EdgeEstimator::Strategy::Normal);
    TEST_ALMOST_EQUAL_ABS(estimator->CalcHeuristic(fromLL, toLL, true /* tightAllowed */), looseHeuristic, kAccuracyEps,
                          (c.m_type));
  }
}
}  // namespace
