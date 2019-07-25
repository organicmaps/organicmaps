#include "testing/testing.hpp"

#include "routing/position_accumulator.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "base/math.hpp"

namespace
{
double constexpr kEps = 1e-10;
double constexpr kEpsMeters = 0.1;

class PositionAccumulatorTest
{
public:
  void PushNextPoint(m2::PointD const & point) { m_positionAccumulator.PushNextPoint(point); }
  void Clear() { m_positionAccumulator.Clear(); }
  m2::PointD GetDirection() const { return m_positionAccumulator.GetDirection(); }

  std::deque<m2::PointD> const & GetPoints() const
  {
    return m_positionAccumulator.GetPointsForTesting();
  }

  double GetTrackLengthM() const { return m_positionAccumulator.GetTrackLengthMForTesting(); }

private:
  PositionAccumulator m_positionAccumulator;
};

UNIT_CLASS_TEST(PositionAccumulatorTest, Smoke)
{
  PushNextPoint({0.0, 0.0});
  PushNextPoint({0.00001, 0.0});
  TEST_EQUAL(GetPoints().size(), 2, ());
  TEST(m2::AlmostEqualAbs(GetDirection(), m2::PointD(0.00001, 0.0), kEps), ());
  TEST(base::AlmostEqualAbs(GetTrackLengthM(), 1.11, kEpsMeters), (GetTrackLengthM()));
}

// Test on using good segments.
UNIT_CLASS_TEST(PositionAccumulatorTest, LastGoodSegment)
{
  double constexpr kStepShortButValid = 0.00001;

  for (signed i = -10; i <= 0; ++i)
    PushNextPoint({kStepShortButValid * i, 0.0});
  TEST_EQUAL(GetPoints().size(), 11, ());
  TEST(m2::AlmostEqualAbs(GetDirection(), m2::PointD(10 * kStepShortButValid, 0.0), kEps),
       (GetDirection()));
  TEST(base::AlmostEqualAbs(GetTrackLengthM(), 11.13, kEpsMeters), (GetTrackLengthM()));

  double constexpr kStepGood = 0.0001;
  for (size_t i = 0; i < 10; ++i)
    PushNextPoint({kStepGood * i, 0.0});

  TEST_EQUAL(GetPoints().size(), 2, ());
  TEST(m2::AlmostEqualAbs(GetDirection(), m2::PointD(kStepGood, 0.0), kEps), (GetDirection()));
  TEST(base::AlmostEqualAbs(GetTrackLengthM(), 11.13, kEpsMeters), (GetTrackLengthM()));
  TEST(m2::AlmostEqualAbs({kStepGood * 8, 0.0}, GetPoints()[0], kEps), ());
  TEST(m2::AlmostEqualAbs({kStepGood * 9, 0.0}, GetPoints()[1], kEps), ());
}

// Test on removing too outdated elements from the position accumulator.
// Adding short but still valid segments.
UNIT_CLASS_TEST(PositionAccumulatorTest, RemovingOutdated)
{
  double constexpr kStep = 0.00001;
  for (size_t i = 0; i < 100; ++i)
    PushNextPoint({kStep * i, 0.0});

  TEST_EQUAL(GetPoints().size(), 64, ());
  TEST(m2::AlmostEqualAbs(GetDirection(), m2::PointD(kStep * 63, 0.0), kEps), (GetDirection()));

  for (size_t i = 36; i < 100; ++i)
  {
    TEST(m2::AlmostEqualAbs({kStep * i, 0.0}, GetPoints()[i - 36], kEps),
         (m2::PointD(kStep * i, 0.0), GetPoints()[i - 36], i));
  }
}

// Test on adding segments longer than |PositionAccumulator::kMaxValidSegmentLengthM|
// and shorter than |PositionAccumulator::kMinValidSegmentLengthM|.
UNIT_CLASS_TEST(PositionAccumulatorTest, LongSegment)
{
  PushNextPoint({0.0, 0.0});
  PushNextPoint({0.001, 0.0}); // Longer than |MaxValidSegmentLengthM|.
  TEST(GetPoints().empty(), ());
  PushNextPoint({0.001001, 0.0}); // Shorter than |kMinValidSegmentLengthM|, but the first one.
  PushNextPoint({0.00102, 0.0});
  PushNextPoint({0.00103, 0.0});
  PushNextPoint({0.001031, 0.0}); // Shorter than |kMinValidSegmentLengthM|, so it's ignored.
  TEST_EQUAL(GetPoints().size(), 3, ());
  TEST(m2::AlmostEqualAbs(GetDirection(), m2::PointD(0.000029, 0.0), kEps), (GetDirection()));
  PushNextPoint({0.00201, 0.0}); // Longer than |PositionAccumulator::kMaxValidSegmentLengthM|.
  TEST_EQUAL(GetPoints().size(), 0, ());
  TEST(m2::AlmostEqualAbs(GetDirection(), m2::PointD(0.0, 0.0), kEps), (GetDirection()));
}
}  // namespace
