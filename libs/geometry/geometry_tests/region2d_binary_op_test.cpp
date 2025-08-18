#include "testing/testing.hpp"

#include "geometry/geometry_tests/test_regions.hpp"
#include "geometry/region2d/binary_operators.hpp"

#include "base/macros.hpp"

#include <vector>

namespace region2d_binary_op_test
{
using namespace std;
using P = m2::PointI;
using R = m2::RegionI;

UNIT_TEST(RegionIntersect_Smoke)
{
  {
    P arr1[] = {P(-2, 1), P(2, 1), P(2, -1), P(-2, -1)};
    P arr2[] = {P(-1, 2), P(1, 2), P(1, -2), P(-1, -2)};

    R r1, r2;
    r1.Assign(arr1, arr1 + ARRAY_SIZE(arr1));
    r2.Assign(arr2, arr2 + ARRAY_SIZE(arr2));

    vector<R> res;
    m2::IntersectRegions(r1, r2, res);

    TEST_EQUAL(res.size(), 1, ());
    TEST_EQUAL(res[0].GetRect(), m2::RectI(-1, -1, 1, 1), ());
  }

  {
    P arr1[] = {P(0, 0), P(1, 1), P(2, 0)};
    P arr2[] = {P(0, 0), P(1, -1), P(2, 0)};

    R r1, r2;
    r1.Assign(arr1, arr1 + ARRAY_SIZE(arr1));
    r2.Assign(arr2, arr2 + ARRAY_SIZE(arr2));

    vector<R> res;
    m2::IntersectRegions(r1, r2, res);

    TEST_EQUAL(res.size(), 0, ());
  }

  {
    P arr1[] = {P(-10, -10), P(10, -10), P(10, 10), P(-10, 10)};
    P arr2[] = {P(-5, -5), P(5, -5), P(5, 5), P(-5, 5)};

    R r1, r2;
    r1.Assign(arr1, arr1 + ARRAY_SIZE(arr1));
    r2.Assign(arr2, arr2 + ARRAY_SIZE(arr2));

    vector<R> res;
    res.push_back(r1);  // do some smoke
    m2::IntersectRegions(r1, r2, res);

    TEST_EQUAL(res.size(), 2, ());
    TEST_EQUAL(res[1].GetRect(), m2::RectI(-5, -5, 5, 5), ());
  }
}

UNIT_TEST(RegionDifference_Smoke)
{
  {
    P arr1[] = {P(-1, 1), P(1, 1), P(1, -1), P(-1, -1)};
    P arr2[] = {P(-2, 2), P(2, 2), P(2, -2), P(-2, -2)};

    R r1, r2;
    r1.Assign(arr1, arr1 + ARRAY_SIZE(arr1));
    r2.Assign(arr2, arr2 + ARRAY_SIZE(arr2));

    vector<R> res;
    m2::DiffRegions(r1, r2, res);
    TEST_EQUAL(res.size(), 0, ());

    m2::DiffRegions(r2, r1, res);

    TEST_EQUAL(res.size(), 1, ());
    TEST_EQUAL(res[0].GetRect(), r2.GetRect(), ());
  }

  {
    P arr1[] = {P(0, 1), P(2, 1), P(2, 0), P(0, 0)};
    P arr2[] = {P(1, 2), P(2, 2), P(2, -1), P(1, -1)};

    R r1, r2;
    r1.Assign(arr1, arr1 + ARRAY_SIZE(arr1));
    r2.Assign(arr2, arr2 + ARRAY_SIZE(arr2));

    vector<R> res;
    m2::DiffRegions(r1, r2, res);

    TEST_EQUAL(res.size(), 1, ());
    TEST_EQUAL(res[0].GetRect(), m2::RectI(0, 0, 1, 1), ());
  }
}

UNIT_TEST(AddRegion_Smoke)
{
  {
    P arr1[] = {{0, 0}, {0, 1}, {1, 1}, {1, 0}};
    P arr2[] = {{2, 2}, {2, 3}, {3, 3}, {3, 2}};

    R r1, r2;
    r1.Assign(arr1, arr1 + ARRAY_SIZE(arr1));
    r2.Assign(arr2, arr2 + ARRAY_SIZE(arr2));

    m2::MultiRegionI res;
    res.push_back(r2);
    m2::AddRegion(r1, res);

    TEST_EQUAL(res.size(), 2, ());
    TEST_EQUAL(m2::Area(res), 2, ());

    res = m2::IntersectRegions(r1, {r2});
    TEST_EQUAL(res.size(), 0, ());
    TEST_EQUAL(m2::Area(res), 0, ());
  }

  {
    P arr1[] = {{0, 0}, {0, 3}, {3, 3}, {3, 0}};
    P arr2[] = {{1, 1}, {1, 2}, {2, 2}, {2, 1}};

    R r1, r2;
    r1.Assign(arr1, arr1 + ARRAY_SIZE(arr1));
    r2.Assign(arr2, arr2 + ARRAY_SIZE(arr2));

    m2::MultiRegionI res;
    res.push_back(r2);
    m2::AddRegion(r1, res);

    TEST_EQUAL(res.size(), 1, ());
    TEST_EQUAL(m2::Area(res), 9, ());

    res = m2::IntersectRegions(r1, {r2});
    TEST_EQUAL(res.size(), 1, ());
    TEST_EQUAL(m2::Area(res), 1, ());
  }
}

UNIT_TEST(RegionIntersect_Floats)
{
  P arr1[] = {{0, 1}, {2, 3}, {3, 2}, {1, 0}};
  P arr2[] = {{0, 2}, {1, 3}, {3, 1}, {2, 0}};

  R r1, r2;
  r1.Assign(arr1, arr1 + ARRAY_SIZE(arr1));
  r2.Assign(arr2, arr2 + ARRAY_SIZE(arr2));

  // Moved diamond as a result with sqrt(2) edge's length and nearest integer coordinates.
  m2::MultiRegionI res;
  m2::IntersectRegions(r1, r2, res);

  TEST_EQUAL(m2::Area(res), 2, ());
}

UNIT_TEST(RegionArea_2Directions)
{
  P arr[] = {
      {1, 1}, {1, 0}, {2, 0}, {2, 1}, {1, 1},  // CCW direction
      {1, 1}, {1, 0}, {0, 0}, {0, 1}, {1, 1}   // CW direction
  };

  R r;
  r.Assign(arr, arr + ARRAY_SIZE(arr));

  // Natural area is 2, but calculated area is 0, because one region with 2 opposite directions.
  TEST_EQUAL(r.CalculateArea(), 0, ());
}

/*
UNIT_TEST(RegionDifference_Data1)
{
  using namespace geom_test;

  vector<R> vec;

  vec.push_back(R());
  vec.back().Assign(arrB1, arrB1 + ARRAY_SIZE(arrB1));

  vec.push_back(R());
  vec.back().Assign(arrB2, arrB2 + ARRAY_SIZE(arrB2));

  vec.push_back(R());
  vec.back().Assign(arrB3, arrB3 + ARRAY_SIZE(arrB3));

  vec.push_back(R());
  vec.back().Assign(arrB4, arrB4 + ARRAY_SIZE(arrB4));

  vec.push_back(R());
  vec.back().Assign(arrB5, arrB5 + ARRAY_SIZE(arrB5));

  vec.push_back(R());
  vec.back().Assign(arrB6, arrB6 + ARRAY_SIZE(arrB6));

  vec.push_back(R());
  vec.back().Assign(arrB7, arrB7 + ARRAY_SIZE(arrB7));

  vec.push_back(R());
  vec.back().Assign(arrB8, arrB8 + ARRAY_SIZE(arrB8));

  vec.push_back(R());
  vec.back().Assign(arrB9, arrB9 + ARRAY_SIZE(arrB9));

  vec.push_back(R());
  vec.back().Assign(arrB10, arrB10 + ARRAY_SIZE(arrB10));

  vec.push_back(R());
  vec.back().Assign(arrB11, arrB11 + ARRAY_SIZE(arrB11));

  vec.push_back(R());
  vec.back().Assign(arrB12, arrB12 + ARRAY_SIZE(arrB12));

  vec.push_back(R());
  vec.back().Assign(arrB13, arrB13 + ARRAY_SIZE(arrB13));

  vec.push_back(R());
  vec.back().Assign(arrB14, arrB14 + ARRAY_SIZE(arrB14));

  vec.push_back(R());
  vec.back().Assign(arrB15, arrB15 + ARRAY_SIZE(arrB15));

  vector<R> res;
  res.push_back(R());
  res.back().Assign(arrMain, arrMain + ARRAY_SIZE(arrMain));

  for (size_t i = 0; i < vec.size(); ++i)
  {
    vector<R> local;

    for (size_t j = 0; j < res.size(); ++j)
      m2::DiffRegions(res[j], vec[i], local);

    local.swap(res);
  }
}
*/
}  // namespace region2d_binary_op_test
