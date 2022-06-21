#include "testing/testing.hpp"

#include "drape/rect_packer.hpp"

UNIT_TEST(SimplePackTest)
{
  dp::RectPacker packer(m2::PointU(32, 32));

  m2::RectU r;

  TEST(packer.Pack(10, 13, r), ());
  TEST_EQUAL(r, m2::RectU(0, 0, 10, 13), ());

  TEST(packer.Pack(18, 8, r), ());
  TEST_EQUAL(r, m2::RectU(10, 0, 28, 8), ());

  TEST(packer.Pack(4, 15, r), ());
  TEST_EQUAL(r, m2::RectU(28, 0, 32, 15), ());

  TEST(packer.Pack(7, 10, r), ());
  TEST(!packer.IsFull(), ());
  TEST_EQUAL(r, m2::RectU(0, 15, 7, 25), ());

  TEST(!packer.Pack(12, 18, r),());
  TEST(packer.IsFull(), ());
  TEST_EQUAL(r, m2::RectU(0, 15, 7, 25), ());
}
