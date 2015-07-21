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

  AStarProgress progress(0, 100);
  progress.Initialize(start, finish);
  TEST_LESS(progress.GetProgressForDirectedAlgo(start), 0.1f, ());
  TEST_LESS(progress.GetProgressForDirectedAlgo(middle), 50.5f, ());
  TEST_GREATER(progress.GetProgressForDirectedAlgo(middle), 49.5f, ());
  TEST_GREATER(progress.GetProgressForDirectedAlgo(finish), 99.9f, ());
}

UNIT_TEST(DirectedAStarDegradationCheck)
{
  m2::PointD start = m2::PointD(0, 1);
  m2::PointD finish = m2::PointD(0, 3);
  m2::PointD middle = m2::PointD(0, 2);

  AStarProgress progress(0, 100);
  progress.Initialize(start, finish);
  auto value1 = progress.GetProgressForDirectedAlgo(middle);
  auto value2 = progress.GetProgressForDirectedAlgo(start);
  TEST_LESS_OR_EQUAL(value1, value2, ());

  progress.Initialize(start, finish);
  auto value3 = progress.GetProgressForDirectedAlgo(start);
  TEST_GREATER_OR_EQUAL(value1, value3, ());
}

UNIT_TEST(RangeCheckTest)
{
  m2::PointD start = m2::PointD(0, 1);
  m2::PointD finish = m2::PointD(0, 3);
  m2::PointD preStart = m2::PointD(0, 0);
  m2::PointD postFinish = m2::PointD(0, 6);

  AStarProgress progress(0, 100);
  progress.Initialize(start, finish);
  TEST_EQUAL(progress.GetProgressForDirectedAlgo(preStart), 0.0, ());
  TEST_EQUAL(progress.GetProgressForDirectedAlgo(postFinish), 0.0, ());
  TEST_EQUAL(progress.GetProgressForDirectedAlgo(finish), 100.0, ());
}

UNIT_TEST(DirectedAStarProgressCheckAtShiftedInterval)
{
  m2::PointD start = m2::PointD(0, 1);
  m2::PointD finish = m2::PointD(0, 3);
  m2::PointD middle = m2::PointD(0, 2);

  AStarProgress progress(50, 250);
  progress.Initialize(start, finish);
  TEST_LESS(progress.GetProgressForDirectedAlgo(start), 50.1f, ());
  TEST_LESS(progress.GetProgressForDirectedAlgo(middle), 150.5f, ());
  TEST_GREATER(progress.GetProgressForDirectedAlgo(middle), 145.5f, ());
  TEST_GREATER(progress.GetProgressForDirectedAlgo(finish), 245.9f, ());
}

UNIT_TEST(BidirectedAStarProgressCheck)
{
  m2::PointD start = m2::PointD(0, 0);
  m2::PointD finish = m2::PointD(0, 4);
  m2::PointD fWave = m2::PointD(0, 1);
  m2::PointD bWave = m2::PointD(0, 3);

  AStarProgress progress(0.0f, 100.0f);
  progress.Initialize(start, finish);
  progress.GetProgressForBidirectedAlgo(fWave, finish);
  float result = progress.GetProgressForBidirectedAlgo(bWave, start);
  TEST_GREATER(result, 49.5, ());
  TEST_LESS(result, 50.5, ());
}
} //  namespace routing_test
