#include "testing/testing.hpp"

#include "routing/position_accumulator.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "base/math.hpp"

namespace
{
using namespace routing;

double constexpr kEps = 1e-10;
double constexpr kEpsMeters = 0.1;

UNIT_CLASS_TEST(PositionAccumulator, Smoke)
{
  PushNextPoint({0.0, 0.0});
  PushNextPoint({0.00002, 0.0});
  TEST_EQUAL(GetPointsForTesting().size(), 2, ());
  TEST(m2::AlmostEqualAbs(GetDirection(), m2::PointD(0.00002, 0.0), kEps), ());
  TEST(base::AlmostEqualAbs(GetTrackLengthMForTesting(), 2.22, kEpsMeters),
       (GetTrackLengthMForTesting()));
}

// Test on using good segments.
UNIT_CLASS_TEST(PositionAccumulator, LastGoodSegment)
{
  double constexpr kStepShortButValid = 0.00002;

  for (signed i = -10; i <= 0; ++i)
    PushNextPoint({kStepShortButValid * i, 0.0});
  TEST_EQUAL(GetPointsForTesting().size(), 11, ());
  TEST(m2::AlmostEqualAbs(GetDirection(), m2::PointD(10 * kStepShortButValid, 0.0), kEps),
       (GetDirection()));
  TEST(base::AlmostEqualAbs(GetTrackLengthMForTesting(), 22.26, kEpsMeters),
       (GetTrackLengthMForTesting()));

  double constexpr kStepGood = 0.0001;
  for (size_t i = 0; i < 10; ++i)
    PushNextPoint({kStepGood * i, 0.0});

  TEST_EQUAL(GetPointsForTesting().size(), 2, ());
  TEST(m2::AlmostEqualAbs(GetDirection(), m2::PointD(kStepGood, 0.0), kEps), (GetDirection()));
  TEST(base::AlmostEqualAbs(GetTrackLengthMForTesting(), 11.13, kEpsMeters),
       (GetTrackLengthMForTesting()));
  TEST(m2::AlmostEqualAbs({kStepGood * 8, 0.0}, GetPointsForTesting()[0], kEps), ());
  TEST(m2::AlmostEqualAbs({kStepGood * 9, 0.0}, GetPointsForTesting()[1], kEps), ());
}

// Test on removing too outdated elements from the position accumulator.
// Adding short but still valid segments.
UNIT_CLASS_TEST(PositionAccumulator, RemovingOutdated)
{
  double constexpr kStep = 0.00002;
  for (size_t i = 0; i < 100; ++i)
    PushNextPoint({kStep * i, 0.0});

  TEST_EQUAL(GetPointsForTesting().size(), 33, ());
  TEST(m2::AlmostEqualAbs(GetDirection(), m2::PointD(kStep * 32, 0.0), kEps), (GetDirection()));

  for (size_t i = 67; i < 100; ++i)
  {
    TEST(m2::AlmostEqualAbs({kStep * i, 0.0}, GetPointsForTesting()[i - 67], kEps),
         (m2::PointD(kStep * i, 0.0), GetPointsForTesting()[i - 67], i));
  }
}

// Test on adding segments longer than |PositionAccumulator::kMaxValidSegmentLengthM|
// and shorter than |PositionAccumulator::kMinValidSegmentLengthM|.
UNIT_CLASS_TEST(PositionAccumulator, LongSegment)
{
  PushNextPoint({0.0, 0.0});
  PushNextPoint({0.001, 0.0}); // Longer than |MaxValidSegmentLengthM|.
  TEST(GetPointsForTesting().empty(), ());
  PushNextPoint({0.001001, 0.0}); // Shorter than |kMinValidSegmentLengthM|, but the first one.
  PushNextPoint({0.00102, 0.0});
  PushNextPoint({0.00104, 0.0});
  PushNextPoint({0.001041, 0.0}); // Shorter than |kMinValidSegmentLengthM|, so it's ignored.
  TEST_EQUAL(GetPointsForTesting().size(), 3, ());
  TEST(m2::AlmostEqualAbs(GetDirection(), m2::PointD(0.000039, 0.0), kEps), (GetDirection()));
  PushNextPoint({0.00201, 0.0}); // Longer than |PositionAccumulator::kMaxValidSegmentLengthM|.
  TEST_EQUAL(GetPointsForTesting().size(), 0, ());
  TEST(m2::AlmostEqualAbs(GetDirection(), m2::PointD(0.0, 0.0), kEps), (GetDirection()));
}
}  // namespace
