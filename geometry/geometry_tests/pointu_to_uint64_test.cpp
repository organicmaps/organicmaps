#include "testing/testing.hpp"
#include "geometry/pointu_to_uint64.hpp"

UNIT_TEST(PointUToUint64_0)
{
  TEST_EQUAL(0, m2::PointUToUint64(m2::PointU(0, 0)), ());
  TEST_EQUAL(m2::PointU(0, 0), m2::Uint64ToPointU(0), ());
}

UNIT_TEST(PointUToUint64_Interlaced)
{
  TEST_EQUAL(0xAAAAAAAAAAAAAAAAULL, m2::PointUToUint64(m2::PointU(0, 0xFFFFFFFF)), ());
  TEST_EQUAL(0x5555555555555555ULL, m2::PointUToUint64(m2::PointU(0xFFFFFFFF, 0)), ());
  TEST_EQUAL(0xAAAAAAAAAAAAAAA8ULL, m2::PointUToUint64(m2::PointU(0, 0xFFFFFFFE)), ());
  TEST_EQUAL(0x5555555555555554ULL, m2::PointUToUint64(m2::PointU(0xFFFFFFFE, 0)), ());
}

UNIT_TEST(PointUToUint64_1bit)
{
  TEST_EQUAL(2, m2::PointUToUint64(m2::PointU(0, 1)), ());
  TEST_EQUAL(m2::PointU(0, 1), m2::Uint64ToPointU(2), ());
  TEST_EQUAL(1, m2::PointUToUint64(m2::PointU(1, 0)), ());
  TEST_EQUAL(m2::PointU(1, 0), m2::Uint64ToPointU(1), ());

  TEST_EQUAL(3ULL << 60, m2::PointUToUint64(m2::PointU(1 << 30, 1 << 30)), ());
  TEST_EQUAL((1ULL << 60) - 1, m2::PointUToUint64(m2::PointU((1 << 30) - 1, (1 << 30) - 1)), ());
}

