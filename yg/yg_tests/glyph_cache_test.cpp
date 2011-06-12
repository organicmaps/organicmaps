#include "../../testing/testing.hpp"
#include "../../qt_tstfrm/macros.hpp"

#include "../glyph_cache.hpp"

UNIT_TEST(GlyphCacheTest_Main)
{
  string const path = GetPlatform().WritableDir();

  yg::GlyphCache cache(yg::GlyphCache::Params(
    (path + "unicode_blocks.txt").c_str(),
    (path + "fonts_whitelist.txt").c_str(),
    (path + "fonts_blacklist.txt").c_str(),
    200000));

  cache.addFont((path + "01_dejavusans.ttf").c_str());
  shared_ptr<yg::GlyphInfo> g1 = cache.getGlyphInfo(yg::GlyphKey('#', 40, true, yg::Color(255, 255, 255, 255)));
//  g1->dump(GetPlatform().WritablePathForFile("#_mask.png").c_str());
  shared_ptr<yg::GlyphInfo> g2 = cache.getGlyphInfo(yg::GlyphKey('#', 40, false, yg::Color(0, 0, 0, 0)));
//  g2->dump(GetPlatform().WritablePathForFile("#.png").c_str());
}
