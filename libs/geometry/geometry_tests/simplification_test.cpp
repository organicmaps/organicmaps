#include "testing/testing.hpp"

#include "geometry/geometry_tests/large_polygon.hpp"
#include "geometry/parametrized_segment.hpp"
#include "geometry/point2d.hpp"
#include "geometry/simplification.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/stl_helpers.hpp"

#include <cstdint>
#include <limits>
#include <vector>

namespace simplification_test
{
using namespace std;

using P = m2::PointD;
using DistanceFn = m2::SquaredDistanceFromSegmentToPoint;
using PointOutput = base::BackInsertFunctor<vector<m2::PointD>>;
using SimplifyFn = void (*)(m2::PointD const *, m2::PointD const *, double, DistanceFn, PointOutput);

struct LargePolylineTestData
{
  static m2::PointD const * m_Data;
  static size_t m_Size;
};

m2::PointD const * LargePolylineTestData::m_Data = LargePolygon::kLargePolygon;
size_t LargePolylineTestData::m_Size = ARRAY_SIZE(LargePolygon::kLargePolygon);

void TestSimplificationSmoke(SimplifyFn simplifyFn)
{
  m2::PointD const points[] = {P(0.0, 1.0), P(2.2, 3.6), P(3.2, 3.6)};
  double const epsilon = 0.1;
  vector<m2::PointD> result, expectedResult(points, points + 3);
  simplifyFn(points, points + 3, epsilon, DistanceFn(), base::MakeBackInsertFunctor(result));
  TEST_EQUAL(result, expectedResult, (epsilon));
}

void TestSimplificationOfLine(SimplifyFn simplifyFn)
{
  m2::PointD const points[] = {P(0.0, 1.0), P(2.2, 3.6)};
  for (double epsilon = numeric_limits<double>::denorm_min(); epsilon < 1000; epsilon *= 2)
  {
    vector<m2::PointD> result, expectedResult(points, points + 2);
    simplifyFn(points, points + 2, epsilon, DistanceFn(), base::MakeBackInsertFunctor(result));
    TEST_EQUAL(result, expectedResult, (epsilon));
  }
}

void TestSimplificationOfPoly(m2::PointD const * points, size_t count, SimplifyFn simplifyFn)
{
  for (double epsilon = 0.00001; epsilon < 0.11; epsilon *= 10)
  {
    vector<m2::PointD> result;
    simplifyFn(points, points + count, epsilon, DistanceFn(), base::MakeBackInsertFunctor(result));
    // LOG(LINFO, ("eps:", epsilon, "size:", result.size()));

    TEST_GREATER(result.size(), 1, ());
    TEST_EQUAL(result.front(), points[0], (epsilon));
    TEST_EQUAL(result.back(), points[count - 1], (epsilon));
    TEST_LESS(result.size(), count, (epsilon));
  }
}

void SimplifyNearOptimal10(m2::PointD const * f, m2::PointD const * l, double e, DistanceFn distFn, PointOutput out)
{
  SimplifyNearOptimal(10, f, l, e, distFn, out);
}

void SimplifyNearOptimal20(m2::PointD const * f, m2::PointD const * l, double e, DistanceFn distFn, PointOutput out)
{
  SimplifyNearOptimal(20, f, l, e, distFn, out);
}

void CheckDPStrict(P const * arr, size_t n, double eps, size_t expectedCount)
{
  vector<P> vec;
  DistanceFn distFn;
  SimplifyDP(arr, arr + n, eps, distFn, AccumulateSkipSmallTrg<DistanceFn, P>(distFn, vec, eps));

  TEST_GREATER(vec.size(), 1, ());
  TEST_EQUAL(arr[0], vec.front(), ());
  TEST_EQUAL(arr[n - 1], vec.back(), ());

  if (expectedCount > 0)
    TEST_EQUAL(expectedCount, vec.size(), ());

  for (size_t i = 2; i < vec.size(); ++i)
  {
    auto const d = DistanceFn();
    TEST_GREATER_OR_EQUAL(d(vec[i - 2], vec[i], vec[i - 1]), eps, ());
  }
}

UNIT_TEST(Simplification_TestDataIsCorrect)
{
  TEST_GREATER_OR_EQUAL(LargePolylineTestData::m_Size, 3, ());
  // This test provides information about the data set size.
  TEST_EQUAL(LargePolylineTestData::m_Size, 5539, ());
}

UNIT_TEST(Simplification_DP_Smoke)
{
  TestSimplificationSmoke(&SimplifyDP<DistanceFn>);
}

UNIT_TEST(Simplification_DP_Line)
{
  TestSimplificationOfLine(&SimplifyDP<DistanceFn>);
}

UNIT_TEST(Simplification_DP_Polyline)
{
  TestSimplificationOfPoly(LargePolylineTestData::m_Data, LargePolylineTestData::m_Size, &SimplifyDP<DistanceFn>);
}

UNIT_TEST(Simplification_Opt_Smoke)
{
  TestSimplificationSmoke(&SimplifyNearOptimal10);
}

UNIT_TEST(Simplification_Opt_Line)
{
  TestSimplificationOfLine(&SimplifyNearOptimal10);
}

UNIT_TEST(Simplification_Opt10_Polyline)
{
  TestSimplificationOfPoly(LargePolylineTestData::m_Data, LargePolylineTestData::m_Size, &SimplifyNearOptimal10);
}

UNIT_TEST(Simplification_Opt20_Polyline)
{
  TestSimplificationOfPoly(LargePolylineTestData::m_Data, LargePolylineTestData::m_Size, &SimplifyNearOptimal20);
}

UNIT_TEST(Simpfication_DP_DegenerateTrg)
{
  P arr1[] = {P(0, 0), P(100, 100), P(100, 500), P(0, 600)};
  CheckDPStrict(arr1, ARRAY_SIZE(arr1), 1.0, 4);

  P arr2[] = {P(0, 0),       P(100, 100),   P(100.1, 150), P(100.2, 200), P(100.3, 250), P(100.4, 300),
              P(100.3, 350), P(100.2, 400), P(100.1, 450), P(100, 500),   P(0, 600)};
  CheckDPStrict(arr2, ARRAY_SIZE(arr2), 1.0, 4);
}
}  // namespace simplification_test
