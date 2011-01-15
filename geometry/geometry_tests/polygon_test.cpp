#include "../../testing/testing.hpp"
#include "../polygon.hpp"
#include "../point2d.hpp"
#include "../../base/macros.hpp"

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

UNIT_TEST(IsDiagonalVisible)
{
  P polyA [] = { P(0,0), P(3,0), P(3,2), P(2,2), P(2,1), P(0,1) };
  vector<P> poly(&polyA[0], &polyA[0] + ARRAY_SIZE(polyA));
  // TODO: Reverse directions.

  TEST(!IsDiagonalVisible(poly.begin(), poly.end(), poly.begin() + 0, poly.begin() + 2), ());
  TEST(!IsDiagonalVisible(poly.begin(), poly.end(), poly.begin() + 0, poly.begin() + 3), ());
  TEST( IsDiagonalVisible(poly.begin(), poly.end(), poly.begin() + 0, poly.begin() + 4), ());
  TEST(!IsDiagonalVisible(poly.begin(), poly.end(), poly.begin() + 5, poly.begin() + 3), ());
  TEST(!IsDiagonalVisible(poly.begin(), poly.end(), poly.begin() + 5, poly.begin() + 2), ());
  TEST( IsDiagonalVisible(poly.begin(), poly.end(), poly.begin() + 5, poly.begin() + 1), ());
  TEST( IsDiagonalVisible(poly.begin(), poly.end(), poly.begin() + 1, poly.begin() + 5), ());
}

UNIT_TEST(FindSingleStrip)
{
  {
    P poly [] = { P(0,0), P(3,0), P(3,2), P(2,2), P(2,1), P(0,1) };
    size_t const n = ARRAY_SIZE(poly);
    TEST_NOT_EQUAL(
          FindSingleStrip(n, IsDiagonalVisibleFunctor<P const *>(&poly[0], &poly[0] + n)), n, ());
  }
  {
    P poly [] = { P(0,0), P(2, -1), P(3,-1), P(3,2), P(2,2), P(2,1), P(0,1) };
    size_t const n = ARRAY_SIZE(poly);
    TEST_EQUAL(
          FindSingleStrip(n, IsDiagonalVisibleFunctor<P const *>(&poly[0], &poly[0] + n)), n, ());
  }
}
