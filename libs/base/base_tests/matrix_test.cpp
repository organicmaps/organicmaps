#include "testing/testing.hpp"

#include "base/matrix.hpp"

UNIT_TEST(Matrix_Inverse_Simple)
{
  math::Matrix<double, 3, 3> m;

  m = math::Identity<double, 3>();

  m(2, 0) = 2;

  math::Matrix<double, 3, 3> m1 = math::Inverse(m);

  TEST_EQUAL((m1 * m).Equal(math::Identity<double, 3>()), true, ());
}

UNIT_TEST(Matrix_Inverse_AllElementsNonZero)
{
  math::Matrix<double, 3, 3> m;
  m(0, 0) = 5;
  m(0, 1) = 3;
  m(0, 2) = 6;
  m(1, 0) = 1;
  m(1, 1) = 7;
  m(1, 2) = 9;
  m(2, 0) = 2;
  m(2, 1) = 13;
  m(2, 2) = 4;

  math::Matrix<double, 3, 3> m1 = math::Inverse(m);

  TEST_EQUAL((m1 * m).Equal(math::Identity<double, 3>()), true, ());
}
