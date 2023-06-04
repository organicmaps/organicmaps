#pragma once

#include "software_renderer/glyph_cache.hpp"

#include "coding/reader.hpp"

#include "base/string_utils.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <ft2build.h>
#include FT_TYPES_H
#include FT_SYSTEM_H
#include FT_FREETYPE_H
#include FT_STROKER_H
#include FT_CACHE_H

namespace software_renderer
{
struct Font
{
  FT_Stream m_fontStream;
  ReaderPtr<Reader> m_fontReader;

  /// information about symbol ranges
  /// ...
  /// constructor
  Font(ReaderPtr<Reader> const & fontReader);
  ~Font();

  FT_Error CreateFaceID(FT_Library library, FT_Face * face);
};

/// Information about single unicode block.
struct UnicodeBlock
{
  std::string m_name;

  /// @{ font matching tuning
  /// whitelist has priority over the blacklist in case of duplicates.
  /// this fonts are promoted to the top of the font list for this block.
  std::vector<std::string> m_whitelist;
  /// this fonts are removed from the font list for this block.
  std::vector<std::string> m_blacklist;
  /// @}

  strings::UniChar m_start;
  strings::UniChar m_end;
  /// sorted indices in m_fonts, from the best to the worst
  std::vector<std::shared_ptr<Font> > m_fonts;
  /// coverage of each font, in symbols
  std::vector<int> m_coverage;

  UnicodeBlock(std::string const & name, strings::UniChar start, strings::UniChar end);
  bool hasSymbol(strings::UniChar sym) const;
};

struct GlyphCacheImpl
{
  FT_Library m_lib;
  FT_Stroker m_stroker;

  FTC_Manager m_manager; //< freetype cache manager for all caches

  FTC_ImageCache m_normalMetricsCache; //< cache of normal glyph metrics
  FTC_ImageCache m_strokedMetricsCache; //< cache of stroked glyph metrics

  FTC_ImageCache m_normalGlyphCache; //< cache of normal glyph images
  FTC_ImageCache m_strokedGlyphCache; //< cache of stroked glyph images

  FTC_CMapCache m_charMapCache; //< cache of glyphID -> glyphIdx mapping

  typedef std::vector<UnicodeBlock> unicode_blocks_t;
  unicode_blocks_t m_unicodeBlocks;
  unicode_blocks_t::iterator m_lastUsedBlock;
  bool m_isDebugging;

  typedef std::vector<std::shared_ptr<Font> > TFonts;
  TFonts m_fonts;

  static FT_Error RequestFace(FTC_FaceID faceID, FT_Library library, FT_Pointer requestData, FT_Face * face);

  void initBlocks(std::string const & fileName);
  void initFonts(std::string const & whiteListFile, std::string const & blackListFile);

  std::vector<std::shared_ptr<Font> > & getFonts(strings::UniChar sym);
  void addFont(char const * fileName);
  void addFonts(std::vector<std::string> const & fontNames);

  int getCharIDX(std::shared_ptr<Font> const & font, strings::UniChar symbolCode);
  std::pair<Font*, int> const getCharIDX(GlyphKey const & key);
  GlyphMetrics const getGlyphMetrics(GlyphKey const & key);
  std::shared_ptr<GlyphBitmap> const getGlyphBitmap(GlyphKey const & key);

  GlyphCacheImpl(GlyphCache::Params const & params);
  ~GlyphCacheImpl();
};
}  // namespace software_renderer
