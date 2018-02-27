#include "testing/testing.hpp"

#include "base/math.hpp"

#include <limits>

#include <boost/math/special_functions/next.hpp>

UNIT_TEST(id)
{
    TEST_EQUAL(my::id(true), true, ());
    TEST_EQUAL(my::id(1), 1.0, ());
    TEST_EQUAL(my::id(1.0), 1, ());
}

UNIT_TEST(SizeAligned)
{
    TEST_EQUAL(my::SizeAligned(0, 1), size_t(0), ());
    TEST_EQUAL(my::SizeAligned(1, 1), size_t(1), ());
    TEST_EQUAL(my::SizeAligned(2, 1), size_t(2), ());
    TEST_EQUAL(my::SizeAligned(3, 1), size_t(3), ());

    TEST_EQUAL(my::SizeAligned(0, 8), size_t(0), ());
    TEST_EQUAL(my::SizeAligned(1, 8), size_t(8), ());
    TEST_EQUAL(my::SizeAligned(2, 8), size_t(8), ());
    TEST_EQUAL(my::SizeAligned(7, 8), size_t(8), ());
    TEST_EQUAL(my::SizeAligned(8, 8), size_t(8), ());
    TEST_EQUAL(my::SizeAligned(9, 8), size_t(16), ());
    TEST_EQUAL(my::SizeAligned(15, 8), size_t(16), ());
    TEST_EQUAL(my::SizeAligned(16, 8), size_t(16), ());
    TEST_EQUAL(my::SizeAligned(17, 8), size_t(24), ());
}

UNIT_TEST(PowUInt)
{
  TEST_EQUAL(my::PowUint(3, 10), 59049, ());
}

UNIT_TEST(AlmostEqualULPs_Smoke)
{
  TEST_ALMOST_EQUAL_ULPS(3.0, 3.0, ());
  TEST_ALMOST_EQUAL_ULPS(+0.0, -0.0, ());

  double const eps = std::numeric_limits<double>::epsilon();
  double const dmax = std::numeric_limits<double>::max();

  TEST_ALMOST_EQUAL_ULPS(1.0 + eps, 1.0, ());
  TEST_ALMOST_EQUAL_ULPS(1.0 - eps, 1.0, ());
  TEST_ALMOST_EQUAL_ULPS(1.0 - eps, 1.0 + eps, ());

  TEST_ALMOST_EQUAL_ULPS(dmax, dmax, ());
  TEST_ALMOST_EQUAL_ULPS(-dmax, -dmax, ());
  TEST_ALMOST_EQUAL_ULPS(dmax/2.0, dmax/2.0, ());
  TEST_ALMOST_EQUAL_ULPS(1.0/dmax, 1.0/dmax, ());
  TEST_ALMOST_EQUAL_ULPS(-1.0/dmax, -1.0/dmax, ());

  TEST(!my::AlmostEqualULPs(1.0, -1.0), ());
  TEST(!my::AlmostEqualULPs(2.0, -2.0), ());
  TEST(!my::AlmostEqualULPs(dmax, -dmax), ());
  TEST(!my::AlmostEqualULPs(0.0, eps), ());
}

UNIT_TEST(AlmostEqual_Smoke)
{
  double const small = 1e-18;
  double const eps = 1e-10;

  TEST(my::AlmostEqualAbs(0.0, 0.0 + small, eps), ());
  TEST(!my::AlmostEqualRel(0.0, 0.0 + small, eps), ());
  TEST(!my::AlmostEqualULPs(0.0, 0.0 + small), ());

  TEST(my::AlmostEqualAbs(1.0, 1.0 + small, eps), ());
  TEST(my::AlmostEqualRel(1.0, 1.0 + small, eps), ());
  TEST(my::AlmostEqualULPs(1.0, 1.0 + small), ());

  TEST(my::AlmostEqualRel(123456789.0, 123456780.0, 1e-7), ());
}

namespace
{

// Returns the next representable floating point value without using conversion to integer.
template <typename FloatT> FloatT NextFloat(FloatT const x, int dir = 1)
{
  return boost::math::float_advance(x, dir);
}

template <typename FloatT> void TestMaxULPs()
{
  for (unsigned int logMaxULPs = 0; logMaxULPs <= 8; ++logMaxULPs)
  {
    unsigned int const maxULPs = (1 << logMaxULPs) - 1;
    for (int base = -1; base <= 1; ++base)
    {
      for (int dir = -1; dir <= 1; dir += 2)
      {
        FloatT const x = base;
        FloatT y = x;
        for (unsigned int i = 0; i <= maxULPs; ++i)
        {
          TEST(my::AlmostEqualULPs(x, y, maxULPs), (x, y, maxULPs, x - y, dir));
          FloatT const nextY = NextFloat(y, dir);
          TEST_NOT_EQUAL(y, nextY, (i, base, dir));
          y = nextY;
        }
        TEST(!my::AlmostEqualULPs(x, y, maxULPs), (x, y, maxULPs, x - y));
      }
    }
  }
}

}

UNIT_TEST(AlmostEqualULPs_MaxULPs_double)
{
  TestMaxULPs<double>();
}

UNIT_TEST(AlmostEqualULPs_MaxULPs_float)
{
  TestMaxULPs<float>();
}

UNIT_TEST(TEST_FLOAT_DOUBLE_EQUAL_macros)
{
  float const fx = 3;
  float const fy = NextFloat(NextFloat(NextFloat(fx)));
  TEST_ALMOST_EQUAL_ULPS(fx, fy, ());
  TEST_NOT_ALMOST_EQUAL_ULPS(fx, 2.0f, ());

  double const dx = 3;
  double const dy = NextFloat(NextFloat(NextFloat(dx)));
  TEST_ALMOST_EQUAL_ULPS(dx, dy, ());
  TEST_NOT_ALMOST_EQUAL_ULPS(dx, 2.0, ());
}

UNIT_TEST(IsIntersect_Intervals)
{
  TEST(my::IsIntersect(0, 100, 100, 200), ());
  TEST(!my::IsIntersect(0, 100, 150, 200), ());
  TEST(my::IsIntersect(0, 100, 50, 150), ());
  TEST(my::IsIntersect(0, 100, 50, 80), ());
  TEST(my::IsIntersect(0, 100, -50, 50), ());
  TEST(my::IsIntersect(0, 100, -50, 0), ());
  TEST(!my::IsIntersect(0, 100, -50, -20), ());
}

UNIT_TEST(GCD_Test)
{
  TEST_EQUAL(my::GCD(6, 3), 3, ());
  TEST_EQUAL(my::GCD(14, 7), 7, ());
  TEST_EQUAL(my::GCD(100, 100), 100, ());
  TEST_EQUAL(my::GCD(7, 3), 1, ());
  TEST_EQUAL(my::GCD(8, 3), 1, ());
  TEST_EQUAL(my::GCD(9, 3), 3, ());
}

UNIT_TEST(LCM_Test)
{
  TEST_EQUAL(my::LCM(6, 3), 6, ());
  TEST_EQUAL(my::LCM(14, 7), 14, ());
  TEST_EQUAL(my::LCM(100, 100), 100, ());
  TEST_EQUAL(my::LCM(7, 3), 21, ());
  TEST_EQUAL(my::LCM(8, 3), 24, ());
  TEST_EQUAL(my::LCM(9, 3), 9, ());
}

UNIT_TEST(Sign_test)
{
  TEST_EQUAL(1, my::Sign(1), ());
  TEST_EQUAL(1, my::Sign(10.4), ());

  TEST_EQUAL(0, my::Sign(0), ());
  TEST_EQUAL(0, my::Sign(0.0), ());

  TEST_EQUAL(-1, my::Sign(-11), ());
  TEST_EQUAL(-1, my::Sign(-10.4), ());
}
