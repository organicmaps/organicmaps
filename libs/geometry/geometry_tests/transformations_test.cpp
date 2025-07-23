#include "geometry/transformations.hpp"
#include "base/matrix.hpp"
#include "geometry/point2d.hpp"
#include "testing/testing.hpp"

UNIT_TEST(Transformations_Shift)
{
  math::Matrix<double, 3, 3> m = math::Shift(math::Identity<double, 3>(), 200.0, 100.0);

  m2::PointD pt = m2::PointD(30, 20) * m;

  TEST(pt.EqualDxDy(m2::PointD(230, 120), 1.0E-10), ());
}

UNIT_TEST(Transformations_ShiftScale)
{
  math::Matrix<double, 3, 3> m = math::Scale(math::Shift(math::Identity<double, 3>(), 100, 100), 2, 3);
  m2::PointD pt = m2::PointD(20, 10) * m;

  TEST(pt.EqualDxDy(m2::PointD(240, 330), 1.0E-10), ());
}

UNIT_TEST(Transformations_Rotate)
{
  math::Matrix<double, 3, 3> m = math::Rotate(math::Identity<double, 3>(), math::pi / 2);
  TEST(m2::PointD(0, 100).EqualDxDy(m2::PointD(100, 0) * m, 1.0E-10), ());
}

UNIT_TEST(Transformations_ShiftScaleRotate)
{
  math::Matrix<double, 3, 3> m =
      math::Rotate(math::Scale(math::Shift(math::Identity<double, 3>(), 100, 100), 2, 3), -math::pi / 2);
  m2::PointD pt = m2::PointD(20, 10) * m;

  TEST(pt.EqualDxDy(m2::PointD(330, -240), 1.0E-10), ());

  math::Matrix<double, 3, 3> invM = math::Inverse(m);

  m2::PointD invPt = m2::PointD(330, -240) * invM;

  TEST(invPt.EqualDxDy(m2::PointD(20, 10), 1.0E-10), ());
}
