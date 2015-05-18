#include "testing/testing.hpp"

#include "drape_frontend/animation/interpolations.hpp"

namespace
{

void IsEqual(m2::AnyRectD const & r1, m2::AnyRectD const & r2)
{
  TEST(r1.LocalZero().EqualDxDy(r2.LocalZero(), 0.00001), ());
  TEST_ALMOST_EQUAL(ang::AngleIn2PI(r1.Angle().val()), ang::AngleIn2PI(r2.Angle().val()), ());
  m2::RectD lR1 = r1.GetLocalRect();
  m2::RectD lR2 = r2.GetLocalRect();
  TEST_ALMOST_EQUAL(lR1.minX(), lR2.minX(), ());
  TEST_ALMOST_EQUAL(lR1.minY(), lR2.minY(), ());
  TEST_ALMOST_EQUAL(lR1.maxX(), lR2.maxX(), ());
  TEST_ALMOST_EQUAL(lR1.maxY(), lR2.maxY(), ());
}

}

UNIT_TEST(MoveRectTest)
{
  double const halfSizeX = 0.5;
  double const halfSizeY = 0.5;

  double const angle = 0.0;
  m2::RectD const sizeRect(-halfSizeX, -halfSizeY, halfSizeX, halfSizeY);
  m2::AnyRectD src(m2::PointD(27.0, 30.0), angle, sizeRect);
  m2::AnyRectD dst(m2::PointD(28.0, 31.0), angle, sizeRect);
  df::InterpolateAnyRect inter(src, dst);
  IsEqual(src, inter.Interpolate(0.0));
  IsEqual(m2::AnyRectD(m2::PointD(27.5, 30.5), angle, sizeRect), inter.Interpolate(0.5));
  IsEqual(dst, inter.Interpolate(1.0));
}

UNIT_TEST(RotateRectTest)
{
  double const halfSizeX = 0.5;
  double const halfSizeY = 0.5;

  m2::PointD zero(27.0, 30.0);
  m2::RectD const sizeRect(-halfSizeX, -halfSizeY, halfSizeX, halfSizeY);
  {
    m2::AnyRectD src(zero, 0.0, sizeRect);
    m2::AnyRectD dst(zero, math::pi2, sizeRect);
    df::InterpolateAnyRect inter(src, dst);
    IsEqual(src, inter.Interpolate(0.0));
    IsEqual(m2::AnyRectD(zero, math::pi4, sizeRect), inter.Interpolate(0.5));
    IsEqual(dst, inter.Interpolate(1.0));
  }
  {
    m2::AnyRectD src(zero, math::pi + math::pi2, sizeRect);
    m2::AnyRectD dst(zero, 0.0, sizeRect);
    df::InterpolateAnyRect inter(src, dst);
    IsEqual(src, inter.Interpolate(0.0));
    IsEqual(m2::AnyRectD(zero, math::pi + math::pi2 + math::pi4, sizeRect), inter.Interpolate(0.5));
    IsEqual(dst, inter.Interpolate(1.0));
  }
}

UNIT_TEST(ScaleRectTest)
{
  m2::PointD zero(27.0, 30.0);
  double const angle = 0.0;

  m2::AnyRectD src(zero, angle, m2::RectD(-0.5, -0.5, 0.5, 0.5));
  m2::AnyRectD dst(zero, angle, m2::RectD(-2.0, -2.0, 2.0, 2.0));

  df::InterpolateAnyRect inter(src, dst);
  IsEqual(src, inter.Interpolate(0.0));
  IsEqual(m2::AnyRectD(zero, angle, m2::RectD(-1.25, -1.25, 1.25, 1.25)), inter.Interpolate(0.5));
  IsEqual(dst, inter.Interpolate(1.0));
}

UNIT_TEST(RectInterpolateTest)
{
  m2::AnyRectD src(m2::PointD(27.0, 36.0), math::pi2, m2::RectD(-1.0, -1.0, 1.0, 1.0));
  m2::AnyRectD dst(m2::PointD(29.0, 30.0), math::pi, m2::RectD(-0.5, -0.5, 0.5, 0.5));
  df::InterpolateAnyRect inter(src, dst);

  IsEqual(src, inter.Interpolate(0.0));
  m2::AnyRectD res(m2::PointD(28.0, 33.0), math::pi2 + math::pi4, m2::RectD(-0.75, -0.75, 0.75, 0.75));
  IsEqual(res, inter.Interpolate(0.5));
  IsEqual(dst, inter.Interpolate(1.0));
}
