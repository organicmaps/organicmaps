#include "testing/testing.hpp"

#include "geometry/point2d.hpp"
#include "geometry/polygon.hpp"
#include "geometry/triangle2d.hpp"

#include "base/macros.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>

namespace polygon_test
{
using namespace std;
using namespace m2::robust;

using P = m2::PointD;

template <typename Iter>
void TestDiagonalVisible(Iter beg, Iter end, Iter i0, Iter i1, bool res)
{
  TEST_EQUAL(IsDiagonalVisible(beg, end, i0, i1), res, ());
  TEST_EQUAL(IsDiagonalVisible(beg, end, i1, i0), res, ());
}

void TestFindStrip(P const * beg, size_t n)
{
  size_t const i = FindSingleStrip(n, IsDiagonalVisibleFunctor<P const *>(beg, beg + n));
  TEST_LESS(i, n, ());

  vector<size_t> test;
  MakeSingleStripFromIndex(i, n, base::MakeBackInsertFunctor(test));

  base::SortUnique(test);
  TEST_EQUAL(test.size(), n, ());
}

void TestFindStripMulti(P const * beg, size_t n)
{
  for (size_t i = 3; i <= n; ++i)
    TestFindStrip(beg, i);
}

template <typename Iter>
void TestPolygonCCW(Iter beg, Iter end)
{
  TEST_EQUAL(m2::robust::CheckPolygonSelfIntersections(beg, end), false, ());

  TEST(IsPolygonCCW(beg, end), ());
  using ReverseIter = reverse_iterator<Iter>;
  TEST(!IsPolygonCCW(ReverseIter(end), ReverseIter(beg)), ());
}

template <typename Iter>
void TestPolygonOrReverseCCW(Iter beg, Iter end)
{
  TEST_EQUAL(m2::robust::CheckPolygonSelfIntersections(beg, end), false, ());

  bool const bForwardCCW = IsPolygonCCW(beg, end);
  using ReverseIter = reverse_iterator<Iter>;
  bool const bReverseCCW = IsPolygonCCW(ReverseIter(end), ReverseIter(beg));
  TEST_NOT_EQUAL(bForwardCCW, bReverseCCW, ());
}

UNIT_TEST(IsSegmentInCone)
{
  TEST(IsSegmentInCone(P(0, 0), P(0, 3), P(-1, -1), P(1, -1)), ());
  TEST(IsSegmentInCone(P(0, 0), P(2, 3), P(-1, -1), P(1, -1)), ());
  TEST(IsSegmentInCone(P(0, 0), P(-3, 3), P(-1, -1), P(1, -1)), ());
  TEST(IsSegmentInCone(P(0, 0), P(-3, 0), P(-1, -1), P(1, -1)), ());
  TEST(IsSegmentInCone(P(0, 0), P(3, 0), P(-1, -1), P(1, -1)), ());
  TEST(!IsSegmentInCone(P(0, 0), P(0, -1), P(-1, -1), P(1, -1)), ());
  TEST(!IsSegmentInCone(P(0, 0), P(1, -3), P(-1, -1), P(1, -1)), ());
  TEST(!IsSegmentInCone(P(0, 0), P(-1, -3), P(-1, -1), P(1, -1)), ());

  TEST(IsSegmentInCone(P(0, 0), P(0, 3), P(-1, 1), P(1, 1)), ());
  TEST(IsSegmentInCone(P(0, 0), P(2, 3), P(-1, 1), P(1, 1)), ());
  TEST(!IsSegmentInCone(P(0, 0), P(-3, 3), P(-1, 1), P(1, 1)), ());
  TEST(!IsSegmentInCone(P(0, 0), P(-3, 0), P(-1, 1), P(1, 1)), ());
  TEST(!IsSegmentInCone(P(0, 0), P(3, 0), P(-1, 1), P(1, 1)), ());
  TEST(!IsSegmentInCone(P(0, 0), P(0, -1), P(-1, 1), P(1, 1)), ());
  TEST(!IsSegmentInCone(P(0, 0), P(1, -3), P(-1, 1), P(1, 1)), ());
  TEST(!IsSegmentInCone(P(0, 0), P(-1, -3), P(-1, 1), P(1, 1)), ());
}

UNIT_TEST(IsDiagonalVisible)
{
  P const poly[] = {P(0, 0), P(3, 0), P(3, 2), P(2, 2), P(2, 1), P(0, 1)};
  P const * b = poly;
  P const * e = poly + ARRAY_SIZE(poly);

  TestDiagonalVisible(b, e, b + 0, b + 1, true);
  TestDiagonalVisible(b, e, b + 0, b + 2, false);
  TestDiagonalVisible(b, e, b + 0, b + 3, false);
  TestDiagonalVisible(b, e, b + 0, b + 4, true);
  TestDiagonalVisible(b, e, b + 0, b + 5, true);
  TestDiagonalVisible(b, e, b + 5, b + 4, true);
  TestDiagonalVisible(b, e, b + 5, b + 3, false);
  TestDiagonalVisible(b, e, b + 5, b + 2, false);
  TestDiagonalVisible(b, e, b + 5, b + 1, true);
}

UNIT_TEST(FindSingleStrip)
{
  {
    P const poly[] = {P(0, 0), P(3, 0), P(3, 2), P(2, 2), P(2, 1), P(0, 1)};
    TestFindStripMulti(poly, ARRAY_SIZE(poly));
  }

  {
    P const poly[] = {P(0, 0), P(2, 0), P(2, -1), P(3, -1), P(3, 2), P(2, 2), P(2, 1), P(0, 1)};
    size_t const n = ARRAY_SIZE(poly);
    TEST_EQUAL(FindSingleStrip(n, IsDiagonalVisibleFunctor<P const *>(poly, poly + n)), n, ());
  }

  {
    // Minsk, Bobryiskaya str., 7
    P const poly[] = {P(53.8926922, 27.5460021), P(53.8926539, 27.5461821), P(53.8926164, 27.5461591),
                      P(53.8925455, 27.5464921), P(53.8925817, 27.5465143), P(53.8925441, 27.5466909),
                      P(53.8923762, 27.5465881), P(53.8925229, 27.5458984)};
    TestFindStrip(poly, ARRAY_SIZE(poly));
  }
}

UNIT_TEST(IsPolygonCCW_Smoke)
{
  P arr1[] = {P(1, 1), P(2, 0), P(3, 2)};
  TestPolygonCCW(arr1, arr1 + ARRAY_SIZE(arr1));

  P arr2[] = {P(0, 0), P(1, 0), P(0, 1)};
  TestPolygonCCW(arr2, arr2 + ARRAY_SIZE(arr2));

  P arr3[] = {P(0, 1), P(1, 1), P(1, 0), P(2, 0), P(2, 1), P(1, 1), P(1, 2), P(0, 2)};
  TestPolygonCCW(arr3, arr3 + ARRAY_SIZE(arr3));
}

UNIT_TEST(IsPolygonCCW_DataSet)
{
  P arr[] = {P(27.3018836975098, 61.7740631103516), P(27.2981071472168, 61.7816162109375),
             P(27.2962188720703, 61.7831611633301), P(27.293815612793, 61.7814445495605),
             P(27.2926139831543, 61.783332824707),  P(27.2919273376465, 61.787109375),
             P(27.2948455810547, 61.7865943908691), P(27.2958755493164, 61.7883110046387),
             P(27.3001670837402, 61.779899597168),  P(27.3036003112793, 61.7771530151367),
             P(27.3015403747559, 61.7747497558594)};

  TestPolygonOrReverseCCW(arr, arr + ARRAY_SIZE(arr));
}

UNIT_TEST(PolygonArea_Smoke)
{
  {
    P arr[] = {P(-1, 0), P(0, 1), P(1, -1)};
    TEST_ALMOST_EQUAL_ULPS(m2::GetTriangleArea(arr[0], arr[1], arr[2]), GetPolygonArea(arr, arr + ARRAY_SIZE(arr)), ());
  }

  {
    P arr[] = {P(-5, -7), P(-3.5, 10), P(7.2, 5), P(14, -6.4)};
    TEST_ALMOST_EQUAL_ULPS(m2::GetTriangleArea(arr[0], arr[1], arr[2]) + m2::GetTriangleArea(arr[2], arr[3], arr[0]),
                           GetPolygonArea(arr, arr + ARRAY_SIZE(arr)), ());
  }
}

// This polygon has self-intersections.
/*
UNIT_TEST(IsPolygonCCW_DataSet2)
{
 P arr[] = { P(0.747119766424532, 61.4800033732131), P(0.747098308752385, 61.4800496413187),
              P(0.747129489432211, 61.4800647287444), P(0.74715195293274, 61.4800191311911),
              P(0.745465178736907, 61.4795420332621), P(0.746959839711849, 61.4802327020841),
              P(0.746994373152972, 61.4802085622029), P(0.747182463060312, 61.479815953858),
              P(0.747314226578283, 61.479873956628), P(0.747109037588444, 61.480298416205),
              P(0.747035947392732, 61.4803450195867), P(0.746934023450081, 61.4803403257209),
              P(0.745422933944894, 61.4796322225403), P(0.745465178736907, 61.4795420332621) };

 TestPolygonOrReverseCCW(arr, arr + ARRAY_SIZE(arr));
}
*/

/*
UNIT_TEST(IsPolygonCCW_DataSet3)
{
 P arr[] = { P(0.738019701780757, 61.1239696304537), P(0.738234278502148, 61.1240028227903),
             P(0.738675837161651, 61.1240276332237), P(0.738234278502148, 61.1240028227903),
             P(0.738019701780757, 61.1239696304537), P(0.737414528371232, 61.1238241206145) };

 TestPolygonOrReverseCCW(arr, arr + ARRAY_SIZE(arr));
}
*/

/*
UNIT_TEST(IsPolygonCCW_DataSet4)
{
  P arr[] = { P(37.368060441099174795, 67.293103344080122952),
P(37.368017525754879671, 67.292797572252140981), P(37.367990703664702323, 67.292969568905391498),
P(37.368060441099174795, 67.293103344080122952), P(37.368017525754879671, 67.292797572252140981),
P(37.368097992025411713, 67.292830094036474975), P(37.368216009222180674, 67.292969568905391498) };
  TestPolygonOrReverseCCW(arr, arr + ARRAY_SIZE(arr));
}
*/
}  // namespace polygon_test
