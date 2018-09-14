#include "testing/testing.hpp"

#include "base/math.hpp"

#include <limits>

#include <boost/math/special_functions/next.hpp>

UNIT_TEST(id)
{
    TEST_EQUAL(base::id(true), true, ());
    TEST_EQUAL(base::id(1), 1.0, ());
    TEST_EQUAL(base::id(1.0), 1, ());
}

UNIT_TEST(SizeAligned)
{
    TEST_EQUAL(base::SizeAligned(0, 1), size_t(0), ());
    TEST_EQUAL(base::SizeAligned(1, 1), size_t(1), ());
    TEST_EQUAL(base::SizeAligned(2, 1), size_t(2), ());
    TEST_EQUAL(base::SizeAligned(3, 1), size_t(3), ());

    TEST_EQUAL(base::SizeAligned(0, 8), size_t(0), ());
    TEST_EQUAL(base::SizeAligned(1, 8), size_t(8), ());
    TEST_EQUAL(base::SizeAligned(2, 8), size_t(8), ());
    TEST_EQUAL(base::SizeAligned(7, 8), size_t(8), ());
    TEST_EQUAL(base::SizeAligned(8, 8), size_t(8), ());
    TEST_EQUAL(base::SizeAligned(9, 8), size_t(16), ());
    TEST_EQUAL(base::SizeAligned(15, 8), size_t(16), ());
    TEST_EQUAL(base::SizeAligned(16, 8), size_t(16), ());
    TEST_EQUAL(base::SizeAligned(17, 8), size_t(24), ());
}

UNIT_TEST(PowUInt)
{
  TEST_EQUAL(base::PowUint(3, 10), 59049, ());
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

  TEST(!base::AlmostEqualULPs(1.0, -1.0), ());
  TEST(!base::AlmostEqualULPs(2.0, -2.0), ());
  TEST(!base::AlmostEqualULPs(dmax, -dmax), ());
  TEST(!base::AlmostEqualULPs(0.0, eps), ());
}

UNIT_TEST(AlmostEqual_Smoke)
{
  double const small = 1e-18;
  double const eps = 1e-10;

  TEST(base::AlmostEqualAbs(0.0, 0.0 + small, eps), ());
  TEST(!base::AlmostEqualRel(0.0, 0.0 + small, eps), ());
  TEST(!base::AlmostEqualULPs(0.0, 0.0 + small), ());

  TEST(base::AlmostEqualAbs(1.0, 1.0 + small, eps), ());
  TEST(base::AlmostEqualRel(1.0, 1.0 + small, eps), ());
  TEST(base::AlmostEqualULPs(1.0, 1.0 + small), ());

  TEST(base::AlmostEqualRel(123456789.0, 123456780.0, 1e-7), ());
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
          TEST(base::AlmostEqualULPs(x, y, maxULPs), (x, y, maxULPs, x - y, dir));
          FloatT const nextY = NextFloat(y, dir);
          TEST_NOT_EQUAL(y, nextY, (i, base, dir));
          y = nextY;
        }
        TEST(!base::AlmostEqualULPs(x, y, maxULPs), (x, y, maxULPs, x - y));
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
  TEST(base::IsIntersect(0, 100, 100, 200), ());
  TEST(!base::IsIntersect(0, 100, 150, 200), ());
  TEST(base::IsIntersect(0, 100, 50, 150), ());
  TEST(base::IsIntersect(0, 100, 50, 80), ());
  TEST(base::IsIntersect(0, 100, -50, 50), ());
  TEST(base::IsIntersect(0, 100, -50, 0), ());
  TEST(!base::IsIntersect(0, 100, -50, -20), ());
}

UNIT_TEST(GCD_Test)
{
  TEST_EQUAL(base::GCD(6, 3), 3, ());
  TEST_EQUAL(base::GCD(14, 7), 7, ());
  TEST_EQUAL(base::GCD(100, 100), 100, ());
  TEST_EQUAL(base::GCD(7, 3), 1, ());
  TEST_EQUAL(base::GCD(8, 3), 1, ());
  TEST_EQUAL(base::GCD(9, 3), 3, ());
}

UNIT_TEST(LCM_Test)
{
  TEST_EQUAL(base::LCM(6, 3), 6, ());
  TEST_EQUAL(base::LCM(14, 7), 14, ());
  TEST_EQUAL(base::LCM(100, 100), 100, ());
  TEST_EQUAL(base::LCM(7, 3), 21, ());
  TEST_EQUAL(base::LCM(8, 3), 24, ());
  TEST_EQUAL(base::LCM(9, 3), 9, ());
}

UNIT_TEST(Sign_test)
{
  TEST_EQUAL(1, base::Sign(1), ());
  TEST_EQUAL(1, base::Sign(10.4), ());

  TEST_EQUAL(0, base::Sign(0), ());
  TEST_EQUAL(0, base::Sign(0.0), ());

  TEST_EQUAL(-1, base::Sign(-11), ());
  TEST_EQUAL(-1, base::Sign(-10.4), ());
}
