#include "testing/testing.hpp"

#include "routing/edge_estimator.hpp"

#include "geometry/point_with_altitude.hpp"

#include <cmath>

namespace
{
double const kTan = 0.1;
geometry::Altitude const kAlt = 100.0;
auto const kPurposes = {routing::EdgeEstimator::Purpose::Weight, routing::EdgeEstimator::Purpose::ETA};
constexpr double kAccuracyEps = 1e-5;

// Climb penalty on plain surface (tangent = 0) must be 1.0 for ETA and Weight estimations.
UNIT_TEST(ClimbPenalty_ZeroTangent)
{
  double const zeroTangent = 0.0;

  for (auto const & purpose : kPurposes)
  {
    TEST_EQUAL(routing::GetCarClimbPenalty(purpose, zeroTangent, kAlt), 1.0, ());
    TEST_EQUAL(routing::GetBicycleClimbPenalty(purpose, zeroTangent, kAlt), 1.0, ());
    TEST_EQUAL(routing::GetPedestrianClimbPenalty(purpose, zeroTangent, kAlt), 1.0, ());
  }
}

// Descent penalty for pedestrians and bicycles must be less then the ascent penalty.
UNIT_TEST(ClimbPenalty_DescentLessThenAscent)
{
  for (auto const & purpose : kPurposes)
  {
    double const ascPenaltyPedestrian = routing::GetPedestrianClimbPenalty(purpose, kTan, kAlt);
    double const descPenaltyPedestrian = routing::GetPedestrianClimbPenalty(purpose, -kTan, kAlt);
    TEST_LESS(descPenaltyPedestrian, ascPenaltyPedestrian, ());

    double const ascPenaltyBicycle = routing::GetBicycleClimbPenalty(purpose, kTan, kAlt);
    double const descPenaltyBicycle = routing::GetBicycleClimbPenalty(purpose, -kTan, kAlt);
    TEST_LESS(descPenaltyBicycle, ascPenaltyBicycle, ());
  }
}

// Descent penalty for cars must be equal to the ascent penalty.
UNIT_TEST(ClimbPenalty_DescentEqualsAscent)
{
  for (auto const & purpose : kPurposes)
  {
    double const ascPenaltyCar = routing::GetCarClimbPenalty(purpose, kTan, kAlt);
    double const descPenaltyCar = routing::GetCarClimbPenalty(purpose, -kTan, kAlt);
    TEST_EQUAL(ascPenaltyCar, 1.0, ());
    TEST_EQUAL(descPenaltyCar, 1.0, ());
  }
}

// Climb penalty high above the sea level (higher then 2.5 km) should be very significant.
UNIT_TEST(ClimbPenalty_HighAboveSeaLevel)
{
  for (auto const & purpose : kPurposes)
  {
    double const penalty2500 = routing::GetPedestrianClimbPenalty(purpose, kTan, 2500);
    double const penalty4000 = routing::GetPedestrianClimbPenalty(purpose, kTan, 4000);
    double const penalty5500 = routing::GetPedestrianClimbPenalty(purpose, kTan, 5500);
    double const penalty7000 = routing::GetPedestrianClimbPenalty(purpose, kTan, 7000);

    TEST_GREATER_OR_EQUAL(penalty2500, 2.0, ());
    TEST_GREATER_OR_EQUAL(penalty4000, penalty2500 + 1.0, ());
    TEST_GREATER_OR_EQUAL(penalty5500, penalty4000 + 1.0, ());
    TEST_GREATER_OR_EQUAL(penalty7000, penalty5500 + 1.0, ());

    double const penalty2500Bicyclce = routing::GetBicycleClimbPenalty(purpose, kTan, 2500);

    TEST_GREATER_OR_EQUAL(penalty2500Bicyclce, 6.0, ());
    TEST_ALMOST_EQUAL_ABS(routing::GetCarClimbPenalty(purpose, kTan, 2500), 1.0, kAccuracyEps, ());
  }
}
}  // namespace
