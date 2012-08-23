#include "../../base/SRC_FIRST.hpp"

#include "equality.hpp"

#include "../../base/macros.hpp"

#include "../../testing/testing.hpp"

#include "../angles.hpp"

using namespace test;

UNIT_TEST(Atan)
{
  double const h4 = math::pi/4.0;

  TEST(is_equal_atan(1, 1, h4), ());
  TEST(is_equal_atan(-1, 1, math::pi - h4), ());
  TEST(is_equal_atan(-1, -1, h4 - math::pi), ());
  TEST(is_equal_atan(1, -1, -h4), ());

  double const hh = atan(1.0/2.0);

  TEST(is_equal_atan(2, 1, hh), ());
  TEST(is_equal_atan(-2, 1, math::pi - hh), ());
  TEST(is_equal_atan(-2, -1, hh - math::pi), ());
  TEST(is_equal_atan(2, -1, -hh), ());
}

namespace
{
  void check_avg(double arr[], size_t n, double v)
  {
    ang::AverageCalc calc;
    for (size_t i = 0; i < n; ++i)
      calc.Add(arr[i]);

    double const avg = calc.GetAverage();
    TEST(is_equal_angle(avg, v), (avg, v));
  }
}

UNIT_TEST(Average)
{
  double const eps = 1.0E-3;

  double arr1[] = { math::pi-eps, -math::pi+eps };
  TEST(is_equal_angle(ang::GetMiddleAngle(arr1[0], arr1[1]), math::pi), ());
  check_avg(arr1, ARRAY_SIZE(arr1), math::pi);

  double arr2[] = { eps, -eps };
  TEST(is_equal_angle(ang::GetMiddleAngle(arr2[0], arr2[1]), 0.0), ());
  check_avg(arr2, ARRAY_SIZE(arr2), 0.0);
}

namespace
{
  bool is_equal(double val0, double val1, double eps)
  {
    return fabs(val0 - val1) < eps;
  }
}

UNIT_TEST(ShortestDistance)
{
  double const eps = 1.0E-3;

  TEST(is_equal(ang::GetShortestDistance(0, math::pi), math::pi, eps), ());
  TEST(is_equal(ang::GetShortestDistance(0, math::pi + 1), -math::pi + 1, eps), ());

  TEST(is_equal(ang::GetShortestDistance(math::pi - 1, 0), -math::pi + 1, eps), ());
  TEST(is_equal(ang::GetShortestDistance(math::pi + 1, 0), math::pi - 1, eps), ());
}
