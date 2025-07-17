#include "testing/testing.hpp"

#include "base/non_intersecting_intervals.hpp"

namespace
{
using namespace base;

UNIT_TEST(NonIntersectingIntervals_1)
{
  NonIntersectingIntervals<int> intervals;

  TEST(intervals.AddInterval(1, 10), ());
  TEST(intervals.AddInterval(11, 15), ());
  // Overlap with [1, 10].
  TEST(!intervals.AddInterval(1, 20), ());

  // Overlap with [11, 15].
  TEST(!intervals.AddInterval(13, 20), ());

  // Overlap with [1, 10] and [11, 15].
  TEST(!intervals.AddInterval(0, 100), ());

  TEST(intervals.AddInterval(100, 150), ());

  // Overlap with [100, 150].
  TEST(!intervals.AddInterval(90, 200), ());
}

UNIT_TEST(NonIntersectingIntervals_2)
{
  NonIntersectingIntervals<int> intervals;

  TEST(intervals.AddInterval(1, 10), ());
  // Overlap with [1, 10].
  TEST(!intervals.AddInterval(2, 9), ());
}

UNIT_TEST(NonIntersectingIntervals_3)
{
  NonIntersectingIntervals<int> intervals;

  TEST(intervals.AddInterval(1, 10), ());
  // Overlap with [1, 10].
  TEST(!intervals.AddInterval(0, 20), ());
}

UNIT_TEST(NonIntersectingIntervals_4)
{
  NonIntersectingIntervals<int> intervals;

  TEST(intervals.AddInterval(1, 10), ());
  // Overlap with [1, 10].
  TEST(!intervals.AddInterval(10, 20), ());
  TEST(!intervals.AddInterval(0, 1), ());
}

UNIT_TEST(NonIntersectingIntervals_5)
{
  NonIntersectingIntervals<int> intervals;

  TEST(intervals.AddInterval(0, 1), ());

  // Overlap with [0, 1].
  TEST(!intervals.AddInterval(1, 2), ());

  TEST(intervals.AddInterval(2, 3), ());
  TEST(intervals.AddInterval(4, 5), ());

  // Overlap with [0, 1] and [2, 3].
  TEST(!intervals.AddInterval(1, 3), ());

  // Overlap with [2, 3].
  TEST(!intervals.AddInterval(2, 2), ());

  // Overlap with [2, 3] and [4, 5].
  TEST(!intervals.AddInterval(2, 5), ());
}
}  // namespace
