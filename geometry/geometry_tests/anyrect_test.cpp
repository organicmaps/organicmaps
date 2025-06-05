#include "testing/testing.hpp"

#include "geometry/any_rect2d.hpp"

#include <array>
#include <cmath>

namespace m2
{
UNIT_TEST(AnyRect_TestConvertTo)
{
  AnyRectD const r(PointD(0, 0), ang::Angle<double>(math::pi / 4), RectD(0, 0, 10, 10));

  PointD const pt1(100, 0);

  double const sqrt2 = sqrt(2.0);
  TEST(r.ConvertTo(pt1).EqualDxDy(PointD(100 / sqrt2, -100 / sqrt2), 10e-5), ());
  TEST(r.ConvertTo(PointD(100, 100)).EqualDxDy(PointD(100 * sqrt2, 0), 10e-5), ());

  AnyRectD r1(PointD(100, 100), ang::Angle<double>(math::pi / 4), RectD(0, 0, 10, 10));

  PointD pt(100, 100 + 50 * sqrt2);

  TEST(r1.ConvertTo(pt).EqualDxDy(PointD(50, 50), 10e-5), ());
}

UNIT_TEST(AnyRect_TestConvertFrom)
{
  AnyRectD const r(PointD(100, 100), ang::Angle<double>(math::pi / 6), RectD(0, 0, 10, 10));

  double const sqrt3 = sqrt(3.0);
  TEST(r.ConvertFrom(PointD(50, 0)).EqualDxDy(PointD(100 + 50 * sqrt3 / 2, 100 + 50 * 1 / 2.0), 10e-5), ());
  TEST(r.ConvertTo(PointD(100 + 50 * sqrt3 / 2, 100 + 50 * 1.0 / 2)).EqualDxDy(PointD(50, 0), 10e-5), ());
}

UNIT_TEST(AnyRect_ZeroRect)
{
  AnyRectD const r0(RectD(0, 0, 0, 0));
  PointD const centerPt(300.0, 300.0);
  AnyRectD const r1(Offset(r0, centerPt));
  TEST_EQUAL(r1.GlobalCenter(), centerPt, ());
  AnyRectD const r2(Inflate(r0, 2.0, 2.0));
  TEST_EQUAL(r2.GetLocalRect(), RectD(-2, -2, 2, 2), ());
}

UNIT_TEST(AnyRect_TestIntersection)
{
  AnyRectD const r0(PointD(93.196, 104.21), ang::Angle<double>(+1.03), RectD(2, 0, 4, 15));
  AnyRectD const r1(PointD(99.713, 116.02), ang::Angle<double>(-1.03), RectD(0, 0, 14, 14));

  std::array<PointD, 4> pts;
  r0.GetGlobalPoints(pts);
  r1.ConvertTo(pts);
  RectD r2(pts[0].x, pts[0].y, pts[0].x, pts[0].y);
  r2.Add(pts[1]);
  r2.Add(pts[2]);
  r2.Add(pts[3]);

  TEST(r1.GetLocalRect().IsIntersect(r2) == false, ());
}

UNIT_TEST(AnyRect_TestIsIntersect)
{
  auto const pi6 = ang::Angle<double>(math::pi / 6);
  auto const pi8 = ang::Angle<double>(math::pi / 8);

  AnyRectD const r0(PointD(100, 100), pi6, RectD(0, 0, 50, 20));
  AnyRectD const r1(PointD(100, 100), pi6, RectD(0, -10, 50, 10));
  AnyRectD const r2(PointD(100, 100), pi6, RectD(0, -21, 50, -1));

  TEST(r0.IsIntersect(r1), ());
  TEST(r1.IsIntersect(r2), ());
  TEST(!r0.IsIntersect(r2), ());
  TEST(r1.IsIntersect(r2), ());

  AnyRectD const r3(PointD(50, 50), pi8, RectD(0, 0, 80, 30));
  TEST(r0.IsIntersect(r3), ());
}

UNIT_TEST(AnyRect_SetSizesToIncludePoint)
{
  AnyRectD rect(PointD(100, 100), ang::Angle<double>(math::pi / 6), RectD(0, 0, 50, 50));

  TEST(!rect.IsPointInside(PointD(0, 0)), ());
  TEST(!rect.IsPointInside(PointD(200, 200)), ());

  rect.SetSizesToIncludePoint(PointD(0, 0));
  TEST(rect.IsPointInside(PointD(0, 0)), ());

  rect.SetSizesToIncludePoint(PointD(200, 200));
  TEST(rect.IsPointInside(PointD(200, 200)), ());
}
}  // namespace m2
