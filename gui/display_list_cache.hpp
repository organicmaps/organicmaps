#pragma once

#include "../std/shared_ptr.hpp"

#include "../graphics/screen.hpp"

namespace gui
{
  class DisplayListCache
  {
  private:

    /// Screen, which should be used for caching
    graphics::Screen * m_CacheScreen;
    /// GlyphCache, which should be used for caching
    graphics::GlyphCache * m_GlyphCache;
    /// Actual cache of glyphs as a display lists
    typedef map<graphics::GlyphKey, shared_ptr<graphics::DisplayList> > TGlyphs;
    TGlyphs m_Glyphs;

    typedef map<string, shared_ptr<graphics::DisplayList> > TSymbols;
    TSymbols m_Symbols;

  public:

    DisplayListCache(graphics::Screen * CacheScreen,
                     graphics::GlyphCache * GlyphCache);

    /// Add element to cache if need be
    void TouchGlyph(graphics::GlyphKey const & key);
    /// Find glyph in cache, caching if needed.
    shared_ptr<graphics::DisplayList> const & FindGlyph(graphics::GlyphKey const & key);
    /// Check, whether the glyph is present in cache.
    bool HasGlyph(graphics::GlyphKey const & key);

    /// @todo refactor to have common functions TouchInfo, FindInfo, HasInfo
    /// taking as example ResourceCache mapInfo, findInfo, hasInfo functions

    /// Add symbol to cache if needed
    void TouchSymbol(char const * name);
    /// Find symbol in cache, caching if needed
    shared_ptr<graphics::DisplayList> const & FindSymbol(char const * name);
    /// Check, whether the display list for specified symbol is present in cache
    bool HasSymbol(char const * name);
  };
}
