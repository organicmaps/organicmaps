#include "../../base/SRC_FIRST.hpp"
#include "../../testing/testing.hpp"
#include "../aa_rect2d.hpp"

UNIT_TEST(AARect_TestConvertTo)
{
  m2::AARectD r(m2::PointD(0, 0), math::pi / 4, m2::RectD(0, 0, 10, 10));

  m2::PointD pt1(100, 0);

  TEST(r.ConvertTo(pt1).EqualDxDy(m2::PointD(100 / sqrt(2), -100 / sqrt(2)), 10e-5), ());
  TEST(r.ConvertTo(m2::PointD(100, 100)).EqualDxDy(m2::PointD(100 * sqrt(2), 0), 10e-5), ());

  m2::AARectD r1(m2::PointD(100, 100), math::pi / 4, m2::RectD(0, 0, 10, 10));

  m2::PointD pt(100, 100 + 50 * sqrt(2));

  TEST(r1.ConvertTo(pt).EqualDxDy(m2::PointD(50, 50), 10e-5), ());
}

UNIT_TEST(AARect_TestConvertFrom)
{
  m2::AARectD r(m2::PointD(100, 100), math::pi / 6, m2::RectD(0, 0, 10, 10));

  TEST(r.ConvertFrom(m2::PointD(50, 0)).EqualDxDy(m2::PointD(100 + 50 * sqrt(3) / 2 , 100 + 50 * 1 / 2.0), 10e-5), ());
  TEST(r.ConvertTo(m2::PointD(100 + 50 * sqrt(3) / 2, 100 + 50 * 1.0 / 2)).EqualDxDy(m2::PointD(50, 0), 10e-5), ());
}

UNIT_TEST(AARect_TestIntersection)
{
  m2::AARectD r0(m2::PointD(93.196, 104.21),  1.03, m2::RectD(2, 0, 4, 15));
  m2::AARectD r1(m2::PointD(99.713, 116.02), -1.03, m2::RectD(0, 0, 14, 14));

  m2::PointD pts[4];
  r0.GetGlobalPoints(pts);
  r1.ConvertTo(pts, 4);
  m2::RectD r2(pts[0].x, pts[0].y, pts[0].x, pts[0].y);
  r2.Add(pts[1]);
  r2.Add(pts[2]);
  r2.Add(pts[3]);

  TEST(r1.GetLocalRect().IsIntersect(r2) == false, ());
}

