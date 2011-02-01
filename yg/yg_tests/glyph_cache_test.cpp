#include "../../testing/testing.hpp"
#include "../../qt_tstfrm/macros.hpp"

#include "../glyph_cache.hpp"

UNIT_TEST(GlyphCacheTest_Main)
{
  yg::GlyphCache cache("", 200000);
  cache.addFont(GetPlatform().ReadPathForFile("dejavusans.ttf").c_str());
  shared_ptr<yg::GlyphInfo> g1 = cache.getGlyph(yg::GlyphKey('#', 40, true));
//  g1->dump(GetPlatform().WritablePathForFile("#_mask.png").c_str());
  shared_ptr<yg::GlyphInfo> g2 = cache.getGlyph(yg::GlyphKey('#', 40, false));
//  g2->dump(GetPlatform().WritablePathForFile("#.png").c_str());
}
