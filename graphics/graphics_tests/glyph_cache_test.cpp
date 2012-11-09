#include "../../testing/testing.hpp"
#include "../../qt_tstfrm/macros.hpp"

#include "../glyph_cache.hpp"

UNIT_TEST(GlyphCacheTest_Main)
{
  graphics::GlyphCache cache(graphics::GlyphCache::Params(
    "unicode_blocks.txt",
    "fonts_whitelist.txt",
    "fonts_blacklist.txt",
    200000,
    false));

  string const path = GetPlatform().WritableDir();

  cache.addFont((path + "01_dejavusans.ttf").c_str());
  shared_ptr<graphics::GlyphInfo> g1 = cache.getGlyphInfo(graphics::GlyphKey('#', 40, true, graphics::Color(255, 255, 255, 255)));
  //g1->dump(GetPlatform().WritablePathForFile("#_mask.png").c_str());
  shared_ptr<graphics::GlyphInfo> g2 = cache.getGlyphInfo(graphics::GlyphKey('#', 40, false, graphics::Color(0, 0, 0, 0)));
  //g2->dump(GetPlatform().WritablePathForFile("#.png").c_str());
}
