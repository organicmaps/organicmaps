#include "testing/testing.hpp"

#include "routing/base/astar_progress.hpp"

namespace routing_test
{
using namespace routing;

UNIT_TEST(DirectedAStarProgressCheck)
{
  m2::PointD start = m2::PointD(0, 1);
  m2::PointD finish = m2::PointD(0, 3);
  m2::PointD middle = m2::PointD(0, 2);

  AStarProgress progress;
  progress.AppendSubProgress({start, finish, 1.0 /* contributionCoef */});
  TEST_LESS(progress.UpdateProgress(start, finish), 0.1f, ());
  TEST_LESS(progress.UpdateProgress(middle, finish), 50.5f, ());
  TEST_GREATER(progress.UpdateProgress(middle, finish), 49.5f, ());
  TEST_GREATER(progress.UpdateProgress(finish, finish), 99.9f, ());
}

UNIT_TEST(DirectedAStarDegradationCheck)
{
  m2::PointD start = m2::PointD(0, 1);
  m2::PointD finish = m2::PointD(0, 3);
  m2::PointD middle = m2::PointD(0, 2);

  AStarProgress progress;
  progress.AppendSubProgress({start, finish, 1.0 /* contributionCoef */});
  auto value1 = progress.UpdateProgress(middle, finish);
  auto value2 = progress.UpdateProgress(start, finish);
  TEST_LESS_OR_EQUAL(value1, value2, ());

  progress.AppendSubProgress({start, finish, 1.0 /* contributionCoef */});
  auto value3 = progress.UpdateProgress(start, finish);
  TEST_GREATER_OR_EQUAL(value1, value3, ());
}

UNIT_TEST(RangeCheckTest)
{
  m2::PointD start = m2::PointD(0, 1);
  m2::PointD finish = m2::PointD(0, 3);
  m2::PointD preStart = m2::PointD(0, 0);
  m2::PointD postFinish = m2::PointD(0, 6);

  AStarProgress progress;
  progress.AppendSubProgress({start, finish, 1.0 /* contributionCoef */});
  TEST_EQUAL(progress.UpdateProgress(preStart, finish), 0.0, ());
  TEST_EQUAL(progress.UpdateProgress(postFinish, finish), 0.0, ());
  TEST_EQUAL(progress.UpdateProgress(finish, finish), 100.0, ());
}

UNIT_TEST(BidirectedAStarProgressCheck)
{
  m2::PointD start = m2::PointD(0, 0);
  m2::PointD finish = m2::PointD(0, 4);
  m2::PointD fWave = m2::PointD(0, 1);
  m2::PointD bWave = m2::PointD(0, 3);

  AStarProgress progress;
  progress.AppendSubProgress({start, finish, 1.0 /* contributionCoef */});
  progress.UpdateProgress(fWave, finish);
  float result = progress.UpdateProgress(bWave, start);
  TEST_GREATER(result, 49.5, ());
  TEST_LESS(result, 50.5, ());
}
} //  namespace routing_test
