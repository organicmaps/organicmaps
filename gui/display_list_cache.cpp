#include "display_list_cache.hpp"

#include "../graphics/display_list.hpp"
#include "../graphics/glyph.hpp"

namespace gui
{
  DisplayListCache::DisplayListCache(graphics::Screen * CacheScreen,
                                     graphics::GlyphCache * GlyphCache)
    : m_CacheScreen(CacheScreen),
      m_GlyphCache(GlyphCache)
  {}

  shared_ptr<graphics::DisplayList> const & DisplayListCache::FindGlyph(graphics::GlyphKey const & key)
  {
    TGlyphs::const_iterator it = m_Glyphs.find(key);

    if (it != m_Glyphs.end())
      return it->second;

    shared_ptr<graphics::DisplayList> & dl = m_Glyphs[key];

    dl.reset(m_CacheScreen->createDisplayList());

    m_CacheScreen->beginFrame();
    m_CacheScreen->setDisplayList(dl.get());

    uint32_t resID = m_CacheScreen->mapInfo(graphics::Glyph::Info(key, m_GlyphCache));
    graphics::Resource const * res = m_CacheScreen->fromID(resID);

    ASSERT(res->m_cat == graphics::Resource::EGlyph, ());
    graphics::Glyph const * glyph = static_cast<graphics::Glyph const *>(res);

    m_CacheScreen->drawGlyph(m2::PointD(0, 0), m2::PointD(0, 0), ang::AngleD(0), 0, glyph, graphics::maxDepth - 10);

    m_CacheScreen->setDisplayList(0);
    m_CacheScreen->endFrame();

    return dl;
  }

  void DisplayListCache::TouchGlyph(graphics::GlyphKey const & key)
  {
    FindGlyph(key);
  }

  bool DisplayListCache::HasGlyph(graphics::GlyphKey const & key)
  {
    return m_Glyphs.find(key) != m_Glyphs.end();
  }
}
