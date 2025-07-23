#include "drape/font_texture.hpp"
#include "testing/testing.hpp"

UNIT_TEST(SimplePackTest)
{
  dp::GlyphPacker packer(m2::PointU(32, 32));

  m2::RectU r;

  TEST(packer.PackGlyph(10, 13, r), ());
  TEST_EQUAL(r, m2::RectU(0, 0, 10, 13), ());

  TEST(packer.PackGlyph(18, 8, r), ());
  TEST_EQUAL(r, m2::RectU(10, 0, 28, 8), ());

  TEST(packer.PackGlyph(4, 15, r), ());
  TEST_EQUAL(r, m2::RectU(28, 0, 32, 15), ());

  TEST(packer.PackGlyph(7, 10, r), ());
  TEST(!packer.IsFull(), ());
  TEST_EQUAL(r, m2::RectU(0, 15, 7, 25), ());

  TEST(!packer.PackGlyph(12, 18, r), ());
  TEST(packer.IsFull(), ());
  TEST_EQUAL(r, m2::RectU(0, 15, 7, 25), ());
}
