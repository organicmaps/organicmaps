#include "testing/testing.hpp"

#include "geometry/robust_orientation.hpp"


typedef m2::PointD P;

bool SegmentsIntersect(P a, P b, P c, P d)
{
  bool const res = m2::robust::SegmentsIntersect(a, b, c, d);
  TEST_EQUAL(res, m2::robust::SegmentsIntersect(a, b, d, c), (a, b, c, d));
  TEST_EQUAL(res, m2::robust::SegmentsIntersect(b, a, c, d), (a, b, c, d));
  TEST_EQUAL(res, m2::robust::SegmentsIntersect(b, a, d, c), (a, b, c, d));
  TEST_EQUAL(res, m2::robust::SegmentsIntersect(c, d, a, b), (a, b, c, d));
  TEST_EQUAL(res, m2::robust::SegmentsIntersect(c, d, b, a), (a, b, c, d));
  TEST_EQUAL(res, m2::robust::SegmentsIntersect(d, c, a, b), (a, b, c, d));
  TEST_EQUAL(res, m2::robust::SegmentsIntersect(d, c, b, a), (a, b, c, d));
  return res;
}

UNIT_TEST(SegmentsIntersect_Collinear)
{
  TEST(!SegmentsIntersect(P(0, 0), P(1, 1), P(2, 3), P(3, 3)), ("Far away"));
  TEST(SegmentsIntersect(P(0, 0), P(1, 1), P(1, 1), P(3, 3)), ("Border intersection"));
  TEST(SegmentsIntersect(P(0, 0), P(2, 2), P(1, 1), P(3, 3)), ("Normal intersection"));
  TEST(SegmentsIntersect(P(0, 0), P(2, 2), P(0, 0), P(3, 3)), ("Border inclusion"));
  TEST(SegmentsIntersect(P(1, 1), P(2, 2), P(0, 0), P(3, 3)), ("Normal inclusion"));
}

UNIT_TEST(SegmentsIntersect_NoIntersection)
{
  TEST(!SegmentsIntersect(P(0, 1), P(2, 5), P(7, 7), P(15, 7)), ("Far away"));
  TEST(!SegmentsIntersect(P(0, 0), P(2, 4), P(1, 1), P(4, 3)), ("Rect intersect, segments don't"));
  TEST(!SegmentsIntersect(P(0, 0), P(8, 8), P(1, 2), P(4, 8)), ("Only one cross product is 0"));
}

UNIT_TEST(SegmentsIntersect_Intersect)
{
  TEST(SegmentsIntersect(P(0, 0), P(2, 4), P(0, 3), P(4, 0)), ("Normal"));
  TEST(SegmentsIntersect(P(1, 2), P(3, 4), P(1, 2), P(5, 1)), ("Fan"));
  TEST(SegmentsIntersect(P(1, 2), P(3, 4), P(1, 2), P(3, -2)), ("Fan"));
  TEST(SegmentsIntersect(P(0, 0), P(2, 2), P(0, 4), P(4, 0)), ("Border"));
}
