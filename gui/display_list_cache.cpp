#include "display_list_cache.hpp"

#include "../graphics/screen.hpp"
#include "../graphics/display_list.hpp"
#include "../graphics/glyph.hpp"
#include "../graphics/depth_constants.hpp"


using namespace graphics;

namespace gui
{
  DisplayListCache::DisplayListCache(Screen * CacheScreen,
                                     GlyphCache * GlyphCache)
    : m_CacheScreen(CacheScreen),
      m_GlyphCache(GlyphCache)
  {}

  shared_ptr<DisplayList> const & DisplayListCache::FindGlyph(GlyphKey const & key)
  {
    TGlyphs::const_iterator it = m_Glyphs.find(key);

    if (it != m_Glyphs.end())
      return it->second;

    shared_ptr<DisplayList> & dl = m_Glyphs[key];

    dl.reset(m_CacheScreen->createDisplayList());

    m_CacheScreen->beginFrame();
    m_CacheScreen->setDisplayList(dl.get());

    uint32_t resID = m_CacheScreen->mapInfo(Glyph::Info(key, m_GlyphCache));
    Resource const * res = m_CacheScreen->fromID(resID);

    ASSERT(res->m_cat == Resource::EGlyph, ());
    Glyph const * glyph = static_cast<Glyph const *>(res);

    m_CacheScreen->drawGlyph(m2::PointD(0, 0), m2::PointD(0, 0), ang::AngleD(0), 0, glyph, maxDepth - 10);

    m_CacheScreen->setDisplayList(0);
    m_CacheScreen->endFrame();

    return dl;
  }

  void DisplayListCache::TouchGlyph(GlyphKey const & key)
  {
    FindGlyph(key);
  }

  bool DisplayListCache::HasGlyph(GlyphKey const & key)
  {
    return m_Glyphs.find(key) != m_Glyphs.end();
  }

  /*
  void DisplayListCache::TouchSymbol(char const * name)
  {
    FindSymbol(name);
  }

  bool DisplayListCache::HasSymbol(char const * name)
  {
    return m_Symbols.find(name) != m_Symbols.end();
  }

  shared_ptr<DisplayList> const & DisplayListCache::FindSymbol(char const * name)
  {
    string s(name);
    TSymbols::const_iterator it = m_Symbols.find(s);

    if (it != m_Symbols.end())
      return it->second;

    shared_ptr<DisplayList> & dl = m_Symbols[s];

    dl.reset(m_CacheScreen->createDisplayList());

    m_CacheScreen->beginFrame();
    m_CacheScreen->setDisplayList(dl.get());

    EPosition pos = EPosAbove;
    if (s == "search-result")
      pos = EPosCenter;

    /// @todo do not cache depth in display list. use separate vertex shader and uniform constant
    /// to specify it while rendering display list.
    m_CacheScreen->drawSymbol(m2::PointD(0, 0), name, pos, poiDepth);

    m_CacheScreen->setDisplayList(0);
    m_CacheScreen->endFrame();

    return dl;
  }
  */
}
