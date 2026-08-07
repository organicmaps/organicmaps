#include "testing/testing.hpp"

#include "routing/edge_estimator.hpp"
#include "routing/geometry.hpp"

#include "routing_common/maxspeed_conversion.hpp"

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

UNIT_TEST(EdgeEstimator_RouteSegmentPenaltyAffectsWeightOnly)
{
  Segment const primary(1 /* mwmId */, 2 /* featureId */, 3 /* segmentIdx */, true /* forward */);
  Segment const unrelated(1 /* mwmId */, 4 /* featureId */, 5 /* segmentIdx */, true /* forward */);
  double constexpr kPenalty = 1.35;

  RoadGeometry::Points const points = {mercator::FromLatLon(0.0, 0.0),   mercator::FromLatLon(0.0, 0.001),
                                       mercator::FromLatLon(0.0, 0.002), mercator::FromLatLon(0.0, 0.003),
                                       mercator::FromLatLon(0.0, 0.004), mercator::FromLatLon(0.0, 0.005),
                                       mercator::FromLatLon(0.0, 0.006)};
  Maxspeed const maxspeed(measurement_utils::Units::Metric, 20 /* forward */, 20 /* backward */);
  RoadGeometry const road(true /* oneWay */, maxspeed, points);
  auto estimator =
      EdgeEstimator::Create(VehicleType::Bicycle, 30.0 /* maxWeightSpeedKMpH */, SpeedKMpH(5.0),
                            nullptr /* trafficStash */, nullptr /* dataSourcePtr */, nullptr /* numMwmIds */);

  double const primaryWeight = estimator->CalcSegmentWeight(primary, road, EdgeEstimator::Purpose::Weight);
  double const primaryEta = estimator->CalcSegmentWeight(primary, road, EdgeEstimator::Purpose::ETA);

  estimator->SetRouteSegmentsPenalty({primary}, kPenalty);

  TEST_ALMOST_EQUAL_ABS(estimator->CalcSegmentWeight(primary, road, EdgeEstimator::Purpose::Weight),
                        primaryWeight * kPenalty, kAccuracyEps, ());
  TEST_ALMOST_EQUAL_ABS(estimator->CalcSegmentWeight(primary.GetReversed(), road, EdgeEstimator::Purpose::Weight),
                        primaryWeight * kPenalty, kAccuracyEps, ());
  TEST_ALMOST_EQUAL_ABS(estimator->CalcSegmentWeight(unrelated, road, EdgeEstimator::Purpose::Weight), primaryWeight,
                        kAccuracyEps, ());
  TEST_ALMOST_EQUAL_ABS(estimator->CalcSegmentWeight(primary, road, EdgeEstimator::Purpose::ETA), primaryEta,
                        kAccuracyEps, ());

  estimator->ClearRouteSegmentsPenalty();
  TEST_ALMOST_EQUAL_ABS(estimator->CalcSegmentWeight(primary, road, EdgeEstimator::Purpose::Weight), primaryWeight,
                        kAccuracyEps, ());
}

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
}  // namespace
