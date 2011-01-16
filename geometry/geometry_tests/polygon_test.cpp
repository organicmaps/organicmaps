#include "../../testing/testing.hpp"

#include "../polygon.hpp"
#include "../point2d.hpp"

#include "../../base/macros.hpp"

#include "../../std/algorithm.hpp"


namespace
{
  typedef m2::PointD P;
}

UNIT_TEST(IsSegmentInCone)
{
  TEST(IsSegmentInCone(P(0,0),  P( 0, 3), P(-1,-1), P(1,-1)), ());
  TEST(IsSegmentInCone(P(0,0),  P( 2, 3), P(-1,-1), P(1,-1)), ());
  TEST(IsSegmentInCone(P(0,0),  P(-3, 3), P(-1,-1), P(1,-1)), ());
  TEST(IsSegmentInCone(P(0,0),  P(-3, 0), P(-1,-1), P(1,-1)), ());
  TEST(IsSegmentInCone(P(0,0),  P( 3, 0), P(-1,-1), P(1,-1)), ());
  TEST(!IsSegmentInCone(P(0,0), P( 0,-1), P(-1,-1), P(1,-1)), ());
  TEST(!IsSegmentInCone(P(0,0), P( 1,-3), P(-1,-1), P(1,-1)), ());
  TEST(!IsSegmentInCone(P(0,0), P(-1,-3), P(-1,-1), P(1,-1)), ());

  TEST(IsSegmentInCone(P(0,0),  P( 0, 3), P(-1,1), P(1,1)), ());
  TEST(IsSegmentInCone(P(0,0),  P( 2, 3), P(-1,1), P(1,1)), ());
  TEST(!IsSegmentInCone(P(0,0), P(-3, 3), P(-1,1), P(1,1)), ());
  TEST(!IsSegmentInCone(P(0,0), P(-3, 0), P(-1,1), P(1,1)), ());
  TEST(!IsSegmentInCone(P(0,0), P( 3, 0), P(-1,1), P(1,1)), ());
  TEST(!IsSegmentInCone(P(0,0), P( 0,-1), P(-1,1), P(1,1)), ());
  TEST(!IsSegmentInCone(P(0,0), P( 1,-3), P(-1,1), P(1,1)), ());
  TEST(!IsSegmentInCone(P(0,0), P(-1,-3), P(-1,1), P(1,1)), ());
}

namespace
{
  template <typename IterT>
  void TestDiagonalVisible(IterT beg, IterT end, IterT i0, IterT i1, bool res)
  {
    TEST_EQUAL ( IsDiagonalVisible(beg, end, i0, i1), res, () );
    TEST_EQUAL ( IsDiagonalVisible(beg, end, i1, i0), res, () );
  }
}

UNIT_TEST(IsDiagonalVisible)
{
  P poly [] = { P(0, 0), P(3, 0), P(3, 2), P(2, 2), P(2, 1), P(0, 1) };
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

namespace 
{
  void TestFindStrip(P const * beg, size_t n)
  {
    size_t const i = FindSingleStrip(n, IsDiagonalVisibleFunctor<P const *>(beg, beg + n));
    TEST_LESS ( i, n, () );

    vector<size_t> test;
    MakeSingleStripFromIndex(i, n, MakeBackInsertFunctor(test));

    sort(test.begin(), test.end());
    unique(test.begin(), test.end());

    TEST_EQUAL ( test.size(), n, () );
  }

  void TestFindStripMulti(P const * beg, size_t n)
  {
    for (size_t i = 3; i <= n; ++i)
      TestFindStrip(beg, i);
  }
}

UNIT_TEST(FindSingleStrip)
{
  {
    P poly[] = { P(0, 0), P(3, 0), P(3, 2), P(2, 2), P(2, 1), P(0, 1) };
    TestFindStripMulti(poly, ARRAY_SIZE(poly));
  }

  {
    P poly[] = { P(0, 0), P(2, 0), P(2, -1), P(3, -1), P(3, 2), P(2, 2), P(2, 1), P(0, 1) };
    size_t const n = ARRAY_SIZE(poly);
    TEST_EQUAL ( FindSingleStrip(n, IsDiagonalVisibleFunctor<P const *>(poly, poly + n)), n, () );
  }

  {
    // Minsk, Bobryiskaya str., 7
    P poly[] = {
      P(53.8926922, 27.5460021),
      P(53.8926539, 27.5461821),
      P(53.8926164, 27.5461591),
      P(53.8925455, 27.5464921),
      P(53.8925817, 27.5465143),
      P(53.8925441, 27.5466909),
      P(53.8923762, 27.5465881),
      P(53.8925229, 27.5458984)
    };
    TestFindStrip(poly, ARRAY_SIZE(poly));
  }
}

namespace 
{
  template <typename IterT> void TestPolygonCCW(IterT beg, IterT end)
  {
    TEST(IsPolygonCCW(beg, end), ());
    reverse(beg, end);
    TEST(!IsPolygonCCW(beg, end), ())
  }
}

UNIT_TEST(IsPolygonCCW)
{
  P arr1[] = { P(1, 1), P(2, 0), P(3, 2) };
  TestPolygonCCW(arr1, arr1 + ARRAY_SIZE(arr1));

  P arr2[] = { P(0, 0), P(1, 0), P(0, 1) };
  TestPolygonCCW(arr2, arr2 + ARRAY_SIZE(arr2));

  P arr3[] = { P(0, 1), P(1, 1), P(1, 0), P(2, 0), P(2, 1), P(1, 1), P(1, 2), P(0, 2) };
  TestPolygonCCW(arr3, arr3 + ARRAY_SIZE(arr3));
}
