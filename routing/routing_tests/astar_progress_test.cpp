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
  progress.Initialise(start, finish);
  TEST_LESS(progress.GetProgressForDirectedAlgo(start), 0.1f, ());
  TEST_LESS(progress.GetProgressForDirectedAlgo(middle), 50.5f, ());
  TEST_GREATER(progress.GetProgressForDirectedAlgo(middle), 49.5f, ());
  TEST_GREATER(progress.GetProgressForDirectedAlgo(finish), 99.9f, ());
}

UNIT_TEST(DirectedAStarProgressCheckAtShiftedInterval)
{
  m2::PointD start = m2::PointD(0, 1);
  m2::PointD finish = m2::PointD(0, 3);
  m2::PointD middle = m2::PointD(0, 2);

  AStarProgress progress(50, 250);
  progress.Initialise(start, finish);
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
  progress.Initialise(start, finish);
  progress.GetProgressForBidirectedAlgo(fWave, finish);
  float result = progress.GetProgressForBidirectedAlgo(bWave, start);
  ASSERT_GREATER(result, 49.5, ());
  ASSERT_LESS(result, 50.5, ());
}
}
