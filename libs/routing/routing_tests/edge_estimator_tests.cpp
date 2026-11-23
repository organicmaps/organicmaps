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

UNIT_TEST(MaxSpeedConstraint_PenaltyApplied)
{
  auto const makeRoad = [](uint16_t speedKmPH)
  {
    return RoadGeometry(true /* oneWay */, Maxspeed(measurement_utils::Units::Metric, speedKmPH, kInvalidSpeed),
                        RoadGeometry::Points({{0.0, 0.0}, {0.0, 0.01}}));
  };

  auto const estimator =
      EdgeEstimator::Create(VehicleType::Car, 200.0, {0.01 /* weight */, kNotUsed /* eta */},
                            nullptr /* trafficStash */, nullptr /* dataSourcePtr */, nullptr /* numMwmIds */);

  Segment const segment(0 /* mwmId */, 0 /* featureId */, 0 /* segmentIdx */, true /* forward */);

  auto const road30 = makeRoad(30);
  auto const road50 = makeRoad(50);
  auto const road70 = makeRoad(70);
  auto const road100 = makeRoad(100);

  auto const calc = [&](RoadGeometry const & road, EdgeEstimator::Purpose purpose)
  { return estimator->CalcSegmentWeight(segment, road, purpose, 0 /* arrivalTime */); };

  double const normalWeight30 = calc(road30, EdgeEstimator::Purpose::Weight);
  double const normalWeight50 = calc(road50, EdgeEstimator::Purpose::Weight);
  double const normalWeight70 = calc(road70, EdgeEstimator::Purpose::Weight);
  double const normalWeight100 = calc(road100, EdgeEstimator::Purpose::Weight);

  double const eta30 = calc(road30, EdgeEstimator::Purpose::ETA);
  double const eta50 = calc(road50, EdgeEstimator::Purpose::ETA);
  double const eta70 = calc(road70, EdgeEstimator::Purpose::ETA);
  double const eta100 = calc(road100, EdgeEstimator::Purpose::ETA);

  // Without max speed constraint, faster roads have lower weight (weight = distance / speed).
  TEST_GREATER(normalWeight30, normalWeight50, ());
  TEST_GREATER(normalWeight50, normalWeight70, ());
  TEST_GREATER(normalWeight70, normalWeight100, ());

  // Set max speed constraint to 50 km/h.
  estimator->SetMaxSpeedKMpH(50.0);
  TEST(estimator->GetMaxSpeedKMpH().has_value(), ());
  TEST_ALMOST_EQUAL_ABS(*estimator->GetMaxSpeedKMpH(), 50.0, kAccuracyEps, ());

  // Roads <= 50 km/h should NOT be penalized.
  TEST_ALMOST_EQUAL_ABS(calc(road30, EdgeEstimator::Purpose::Weight), normalWeight30, kAccuracyEps, ());
  TEST_ALMOST_EQUAL_ABS(calc(road50, EdgeEstimator::Purpose::Weight), normalWeight50, kAccuracyEps, ());

  // Roads > 50 km/h should be heavily penalized in Purpose::Weight.
  double const penalizedWeight70 = calc(road70, EdgeEstimator::Purpose::Weight);
  double const penalizedWeight100 = calc(road100, EdgeEstimator::Purpose::Weight);

  TEST_GREATER(penalizedWeight70, normalWeight70, ());
  TEST_GREATER(penalizedWeight100, normalWeight100, ());
  // Under the penalty, 70 km/h and 100 km/h roads become much more expensive than a 50 km/h road.
  TEST_GREATER(penalizedWeight70, normalWeight50, ());
  TEST_GREATER(penalizedWeight100, penalizedWeight70, ());

  // ETA should remain unaffected by the max speed constraint.
  TEST_ALMOST_EQUAL_ABS(calc(road30, EdgeEstimator::Purpose::ETA), eta30, kAccuracyEps, ());
  TEST_ALMOST_EQUAL_ABS(calc(road50, EdgeEstimator::Purpose::ETA), eta50, kAccuracyEps, ());
  TEST_ALMOST_EQUAL_ABS(calc(road70, EdgeEstimator::Purpose::ETA), eta70, kAccuracyEps, ());
  TEST_ALMOST_EQUAL_ABS(calc(road100, EdgeEstimator::Purpose::ETA), eta100, kAccuracyEps, ());

  // Resetting max speed removes the penalties.
  estimator->SetMaxSpeedKMpH(std::nullopt);
  TEST(!estimator->GetMaxSpeedKMpH().has_value(), ());
  TEST_ALMOST_EQUAL_ABS(calc(road70, EdgeEstimator::Purpose::Weight), normalWeight70, kAccuracyEps, ());
  TEST_ALMOST_EQUAL_ABS(calc(road100, EdgeEstimator::Purpose::Weight), normalWeight100, kAccuracyEps, ());
}

// United States: Interstate (65-70 mph) vs State Highway / Arterial (45-55 mph) vs Local (25 mph).
// Tests imperial speeds converted to metric km/h, matching GraphHopper custom profile and OSRM
// motorway penalty expectations.
UNIT_TEST(MaxSpeedConstraint_US_InterstateVsArterial)
{
  auto const makeRoad = [](measurement_utils::Units units, uint16_t speed)
  {
    return RoadGeometry(true /* oneWay */, Maxspeed(units, speed, kInvalidSpeed),
                        RoadGeometry::Points({{0.0, 0.0}, {0.0, 0.01}}));
  };

  auto const estimator =
      EdgeEstimator::Create(VehicleType::Car, 200.0, {0.01 /* weight */, kNotUsed /* eta */},
                            nullptr /* trafficStash */, nullptr /* dataSourcePtr */, nullptr /* numMwmIds */);

  Segment const segment(0, 0, 0, true);
  auto const calc = [&](RoadGeometry const & road, EdgeEstimator::Purpose purpose)
  { return estimator->CalcSegmentWeight(segment, road, purpose, 0 /* arrivalTime */); };

  // Imperial speeds (mph) as typically posted across the US:
  auto const interstate70 = makeRoad(measurement_utils::Units::Imperial, 70);
  auto const interstate65 = makeRoad(measurement_utils::Units::Imperial, 65);
  auto const stateHwy55 = makeRoad(measurement_utils::Units::Imperial, 55);
  auto const stateHwy45 = makeRoad(measurement_utils::Units::Imperial, 45);
  auto const local25 = makeRoad(measurement_utils::Units::Imperial, 25);

  // Unconstrained: faster roads have strictly lower weights.
  TEST_GREATER(calc(local25, EdgeEstimator::Purpose::Weight), calc(stateHwy45, EdgeEstimator::Purpose::Weight), ());
  TEST_GREATER(calc(stateHwy45, EdgeEstimator::Purpose::Weight), calc(stateHwy55, EdgeEstimator::Purpose::Weight), ());
  TEST_GREATER(calc(stateHwy55, EdgeEstimator::Purpose::Weight), calc(interstate65, EdgeEstimator::Purpose::Weight), ());
  TEST_GREATER(calc(interstate65, EdgeEstimator::Purpose::Weight), calc(interstate70, EdgeEstimator::Purpose::Weight), ());

  // Cap speed at 50 mph (~80.5 km/h).
  // Roads above 50 mph (65 and 70 mph) receive progressive penalties, reversing the preference
  // to favor the 45 mph arterial over the fast Interstate.
  double const k50MphInKmPH = measurement_utils::MiphToKmph(50.0);
  estimator->SetMaxSpeedKMpH(k50MphInKmPH);

  double const normalWeight45 = calc(stateHwy45, EdgeEstimator::Purpose::Weight);
  double const normalWeight25 = calc(local25, EdgeEstimator::Purpose::Weight);

  // Roads <= 50 mph retain their exact unpenalized weight.
  TEST_ALMOST_EQUAL_ABS(calc(stateHwy45, EdgeEstimator::Purpose::Weight), normalWeight45, kAccuracyEps, ());
  TEST_ALMOST_EQUAL_ABS(calc(local25, EdgeEstimator::Purpose::Weight), normalWeight25, kAccuracyEps, ());

  // 65 and 70 mph Interstates are heavily penalized beyond the 45 mph arterial.
  double const penalized65 = calc(interstate65, EdgeEstimator::Purpose::Weight);
  double const penalized70 = calc(interstate70, EdgeEstimator::Purpose::Weight);
  TEST_GREATER(penalized65, normalWeight45, ());
  TEST_GREATER(penalized70, penalized65, ());

  // ETA remains invariant: user gets realistic travel time.
  double const normalEta70 = calc(interstate70, EdgeEstimator::Purpose::ETA);
  double const normalEta65 = calc(interstate65, EdgeEstimator::Purpose::ETA);
  TEST_ALMOST_EQUAL_ABS(calc(interstate70, EdgeEstimator::Purpose::ETA), normalEta70, kAccuracyEps, ());
  TEST_ALMOST_EQUAL_ABS(calc(interstate65, EdgeEstimator::Purpose::ETA), normalEta65, kAccuracyEps, ());
}

// Europe / Germany: Autobahn (130 km/h) vs Landstraße (100 km/h) vs Bundesstraße (80 km/h) vs Innerorts (50 km/h).
// Simulates trailer hauling, novice driver, or eco-profile capping speed at 80 km/h.
UNIT_TEST(MaxSpeedConstraint_Europe_AutobahnVsSecondary)
{
  auto const makeRoad = [](uint16_t speedKmPH)
  {
    return RoadGeometry(true /* oneWay */, Maxspeed(measurement_utils::Units::Metric, speedKmPH, kInvalidSpeed),
                        RoadGeometry::Points({{0.0, 0.0}, {0.0, 0.01}}));
  };

  auto const estimator =
      EdgeEstimator::Create(VehicleType::Car, 200.0, {0.01, kNotUsed}, nullptr, nullptr, nullptr);

  Segment const segment(0, 0, 0, true);
  auto const calc = [&](RoadGeometry const & road, EdgeEstimator::Purpose purpose)
  { return estimator->CalcSegmentWeight(segment, road, purpose, 0); };

  auto const autobahn130 = makeRoad(130);
  auto const landstrasse100 = makeRoad(100);
  auto const bundesstrasse80 = makeRoad(80);
  auto const innerorts50 = makeRoad(50);
  auto const tempo30 = makeRoad(30);

  // Unconstrained weights: 130 km/h < 100 km/h < 80 km/h < 50 km/h < 30 km/h.
  TEST_GREATER(calc(tempo30, EdgeEstimator::Purpose::Weight), calc(innerorts50, EdgeEstimator::Purpose::Weight), ());
  TEST_GREATER(calc(innerorts50, EdgeEstimator::Purpose::Weight), calc(bundesstrasse80, EdgeEstimator::Purpose::Weight), ());
  TEST_GREATER(calc(bundesstrasse80, EdgeEstimator::Purpose::Weight), calc(landstrasse100, EdgeEstimator::Purpose::Weight), ());
  TEST_GREATER(calc(landstrasse100, EdgeEstimator::Purpose::Weight), calc(autobahn130, EdgeEstimator::Purpose::Weight), ());

  // Cap speed at 80 km/h.
  estimator->SetMaxSpeedKMpH(80.0);

  double const normalWeight80 = calc(bundesstrasse80, EdgeEstimator::Purpose::Weight);
  double const normalWeight50 = calc(innerorts50, EdgeEstimator::Purpose::Weight);
  double const normalWeight30 = calc(tempo30, EdgeEstimator::Purpose::Weight);

  // Roads <= 80 km/h stay at normal cost.
  TEST_ALMOST_EQUAL_ABS(calc(bundesstrasse80, EdgeEstimator::Purpose::Weight), normalWeight80, kAccuracyEps, ());
  TEST_ALMOST_EQUAL_ABS(calc(innerorts50, EdgeEstimator::Purpose::Weight), normalWeight50, kAccuracyEps, ());
  TEST_ALMOST_EQUAL_ABS(calc(tempo30, EdgeEstimator::Purpose::Weight), normalWeight30, kAccuracyEps, ());

  // Autobahn (130) and Landstraße (100) are penalized above the 80 km/h Bundesstraße.
  double const penalized100 = calc(landstrasse100, EdgeEstimator::Purpose::Weight);
  double const penalized130 = calc(autobahn130, EdgeEstimator::Purpose::Weight);
  TEST_GREATER(penalized100, normalWeight80, ());
  TEST_GREATER(penalized130, penalized100, ());

  // ETA is unaffected.
  TEST_ALMOST_EQUAL_ABS(calc(autobahn130, EdgeEstimator::Purpose::ETA),
                        autobahn130.GetDistance(0) / measurement_utils::KmphToMps(130), 1e-4, ());
}

// United Kingdom: National Speed Limits (70 mph Motorway, 60 mph Single Carriageway, 30 mph Built-up).
UNIT_TEST(MaxSpeedConstraint_UK_NationalSpeedLimits)
{
  auto const makeRoad = [](uint16_t speedMph)
  {
    return RoadGeometry(true /* oneWay */, Maxspeed(measurement_utils::Units::Imperial, speedMph, kInvalidSpeed),
                        RoadGeometry::Points({{0.0, 0.0}, {0.0, 0.01}}));
  };

  auto const estimator =
      EdgeEstimator::Create(VehicleType::Car, 200.0, {0.01, kNotUsed}, nullptr, nullptr, nullptr);

  Segment const segment(0, 0, 0, true);
  auto const calc = [&](RoadGeometry const & road, EdgeEstimator::Purpose purpose)
  { return estimator->CalcSegmentWeight(segment, road, purpose, 0); };

  auto const motorway70 = makeRoad(70);
  auto const singleCarriageway60 = makeRoad(60);
  auto const builtUp30 = makeRoad(30);

  // Driver cap at 50 mph (~80.5 km/h).
  estimator->SetMaxSpeedKMpH(measurement_utils::MiphToKmph(50.0));

  double const weight30 = calc(builtUp30, EdgeEstimator::Purpose::Weight);
  double const penalized60 = calc(singleCarriageway60, EdgeEstimator::Purpose::Weight);
  double const penalized70 = calc(motorway70, EdgeEstimator::Purpose::Weight);

  // Both roads above 50 mph are penalized proportionally.
  TEST_GREATER(penalized70, penalized60, ());
  // 30 mph road is not penalized.
  estimator->SetMaxSpeedKMpH(std::nullopt);
  TEST_ALMOST_EQUAL_ABS(calc(builtUp30, EdgeEstimator::Purpose::Weight), weight30, kAccuracyEps, ());
}

// Australia: NSW P-plate (provisional driver) statutory cap of 90 km/h.
// M1 Motorway (110 km/h) vs A1 Highway (100 km/h) vs Regional (80 km/h) vs Urban (50 km/h).
UNIT_TEST(MaxSpeedConstraint_Australia_ProvisionalDriverPPlate)
{
  auto const makeRoad = [](uint16_t speedKmPH)
  {
    return RoadGeometry(true /* oneWay */, Maxspeed(measurement_utils::Units::Metric, speedKmPH, kInvalidSpeed),
                        RoadGeometry::Points({{0.0, 0.0}, {0.0, 0.01}}));
  };

  auto const estimator =
      EdgeEstimator::Create(VehicleType::Car, 200.0, {0.01, kNotUsed}, nullptr, nullptr, nullptr);

  Segment const segment(0, 0, 0, true);
  auto const calc = [&](RoadGeometry const & road, EdgeEstimator::Purpose purpose)
  { return estimator->CalcSegmentWeight(segment, road, purpose, 0); };

  auto const motorway110 = makeRoad(110);
  auto const highway100 = makeRoad(100);
  auto const regional80 = makeRoad(80);
  auto const urban50 = makeRoad(50);

  // Australian NSW P1 provisional licence holders are restricted to a maximum speed of 90 km/h by law.
  estimator->SetMaxSpeedKMpH(90.0);

  double const normalWeight80 = calc(regional80, EdgeEstimator::Purpose::Weight);
  double const normalWeight50 = calc(urban50, EdgeEstimator::Purpose::Weight);

  // 80 and 50 km/h roads incur zero penalty.
  TEST_ALMOST_EQUAL_ABS(calc(regional80, EdgeEstimator::Purpose::Weight), normalWeight80, kAccuracyEps, ());
  TEST_ALMOST_EQUAL_ABS(calc(urban50, EdgeEstimator::Purpose::Weight), normalWeight50, kAccuracyEps, ());

  // 100 km/h and 110 km/h roads are penalized.
  double const penalized100 = calc(highway100, EdgeEstimator::Purpose::Weight);
  double const penalized110 = calc(motorway110, EdgeEstimator::Purpose::Weight);
  TEST_GREATER(penalized100, normalWeight80, ());
  TEST_GREATER(penalized110, penalized100, ());
}

// Bottleneck / Sole Connector (GraphHopper & OSRM soft penalty comparison):
// When a 110 km/h bridge or expressway connector is the ONLY route between two regions,
// hard avoidance (like avoid motorways) fails with "no route found".
// Max speed constraint applies a finite penalty, guaranteeing the route still succeeds.
UNIT_TEST(MaxSpeedConstraint_BottleneckSoftPenalty_AllowsPassage)
{
  auto const makeRoad = [](uint16_t speedKmPH)
  {
    return RoadGeometry(true /* oneWay */, Maxspeed(measurement_utils::Units::Metric, speedKmPH, kInvalidSpeed),
                        RoadGeometry::Points({{0.0, 0.0}, {0.0, 0.01}}));
  };

  auto const estimator =
      EdgeEstimator::Create(VehicleType::Car, 200.0, {0.01, kNotUsed}, nullptr, nullptr, nullptr);

  Segment const segment(0, 0, 0, true);
  auto const calc = [&](RoadGeometry const & road, EdgeEstimator::Purpose purpose)
  { return estimator->CalcSegmentWeight(segment, road, purpose, 0); };

  auto const bridge110 = makeRoad(110);
  estimator->SetMaxSpeedKMpH(50.0);

  double const penalizedWeight = calc(bridge110, EdgeEstimator::Purpose::Weight);
  // Weight is finite and strictly valid (not infinity or prohibited).
  TEST(!std::isinf(penalizedWeight), ());
  TEST_GREATER(penalizedWeight, 0.0, ());

  // ETA matches physical bridge crossing time.
  double const eta = calc(bridge110, EdgeEstimator::Purpose::ETA);
  TEST_ALMOST_EQUAL_ABS(eta, bridge110.GetDistance(0) / measurement_utils::KmphToMps(110), 1e-4, ());
}
}  // namespace
