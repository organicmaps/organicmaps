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

// CanBePacked is a dry run; it must reach the same verdict as a sequence of PackGlyph calls
// would. Empty-packer and over-saturation cases are obvious; the row-wrap case below pins a
// real bug fixed by sharing the row-flow helper between PackGlyph and CanBePacked.
UNIT_TEST(GlyphPacker_CanBePacked_BasicCases)
{
  dp::GlyphPacker packer(m2::PointU(32, 32));

  // 9 cells of 10x10 do not fit (3x3 grid would need 30x30 plus alignment leaves no fourth row),
  // so anything beyond 9 is over-saturated. 8 cells fit comfortably.
  TEST(packer.CanBePacked(8, 10, 10), ("8 cells of 10x10 fit in a 32x32 atlas"));
  TEST(!packer.CanBePacked(10, 10, 10), ("10 cells of 10x10 do not fit in a 32x32 atlas"));

  // Saturation: asking for more than the atlas can hold returns false without changing state.
  TEST(!packer.CanBePacked(100, 16, 16), ("100 cells of 16x16 cannot fit in 32x32"));

  // CanBePacked is const: cursor must be untouched, so a real PackGlyph still places at (0, 0).
  m2::RectU r;
  TEST(packer.PackGlyph(10, 10, r), ());
  TEST_EQUAL(r, m2::RectU(0, 0, 10, 10), ("dry run did not advance the cursor"));
}

// Regression: previously CanBePacked carried the previous row's yStep across a row wrap while
// PackGlyph reset it to 0. CanBePacked over-rejected, causing the texture manager to spawn an
// extra atlas when the existing one had room. After unifying via AdvanceCursor, CanBePacked
// agrees with what PackGlyph actually does.
UNIT_TEST(GlyphPacker_CanBePacked_ResetsStepOnRowWrap)
{
  dp::GlyphPacker packer(m2::PointU(100, 100));

  // Pack a tall glyph; advances yStep to 80 and leaves cursor at (50, 0).
  m2::RectU r;
  TEST(packer.PackGlyph(50, 80, r), ());
  TEST_EQUAL(r, m2::RectU(0, 0, 50, 80), ());

  // After a row wrap the next row should start fresh: yStep reset to 0, then grown by the cells
  // placed in row 2. Row-2 capacity is the remaining 20 px of vertical space, so 16+ cells of
  // 10x10 fit (5 in row 1 right of the tall glyph, 10 in row 2 at y=80..90, plus more in row 3
  // at y=90..100). Old buggy CanBePacked carried yStep=80 across the wrap, claimed y went from
  // 80 to 160, and rejected the 16th cell.
  TEST(packer.CanBePacked(16, 10, 10), ("CanBePacked must reset yStep on row wrap (regression)"));

  // Sanity: mirror the dry run with real packs and confirm none over-flow.
  for (int i = 0; i < 16; ++i)
    TEST(packer.PackGlyph(10, 10, r), ("PackGlyph #", i));
  TEST(!packer.IsFull(), ("packer should not be full after 16 small glyphs"));
}
