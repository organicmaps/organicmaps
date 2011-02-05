#pragma once

#include "glyph_cache.hpp"
#include "ft2_debug.hpp"

#include "../base/memory_mapped_file.hpp"

#include "../std/string.hpp"
#include "../std/vector.hpp"
#include "../std/shared_ptr.hpp"


namespace yg
{
  struct Font
  {
    string m_name;

    MemoryMappedFile m_fontData;

    /// information about symbol ranges
    /// ...
    /// constructor
    Font(char const * name);

    FT_Error CreateFaceID(FT_Library library, FT_Face * face);
  };

  /// Information about single unicode block.
  struct UnicodeBlock
  {
    string m_name;

    /// @{ Font Tuning
    /// whitelist has priority over the blacklist in case of duplicates.
    /// this fonts are promoted to the top of the font list for this block.
    vector<string> m_whitelist;
    /// this fonts are removed from the font list for this block.
    vector<string> m_blacklist;
    /// @}

    uint32_t m_start;
    uint32_t m_end;
    /// sorted indices in m_fonts, from the best to the worst
    vector<shared_ptr<Font> > m_fonts;
    /// coverage of each font, in symbols
    vector<int> m_coverage;

    UnicodeBlock(string const & name, uint32_t start, uint32_t end);
    bool hasSymbol(uint16_t sym) const;
  };

  struct GlyphCacheImpl
  {
    FT_Library m_lib;
    FT_Stroker m_stroker;

    FTC_Manager m_manager;

    FTC_ImageCache m_glyphMetricsCache;
    FTC_ImageCache m_strokedGlyphCache;
    FTC_ImageCache m_normalGlyphCache;

    FTC_CMapCache m_charMapCache;

    typedef vector<UnicodeBlock> unicode_blocks_t;
    unicode_blocks_t m_unicodeBlocks;
    unicode_blocks_t::iterator m_lastUsedBlock;

    typedef vector<shared_ptr<Font> > TFonts;
    TFonts m_fonts;

    static FT_Error RequestFace(FTC_FaceID faceID, FT_Library library, FT_Pointer requestData, FT_Face * face);

    void initBlocks(string const & fileName);
    void initFonts(string const & whiteListFile, string const & blackListFile);

    vector<shared_ptr<Font> > & getFonts(uint16_t sym);
    void addFont(char const * fileName);
    void addFonts(vector<string> const & fontNames);

    GlyphCacheImpl(GlyphCache::Params const & params);
    ~GlyphCacheImpl();
  };
}
