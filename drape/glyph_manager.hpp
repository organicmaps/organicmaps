#pragma once

#include "base/string_utils.hpp"

#include "drape/glyph.hpp"

#include <string>
#include <vector>

namespace dp
{
struct UnicodeBlock;

// TODO(AB): Move to a separate file?
namespace text
{
struct GlyphMetrics
{
  int16_t m_font;
  uint16_t m_glyphId;
  // TODO(AB): Store original font units or floats?
  int32_t m_xOffset;
  int32_t m_yOffset;
  int32_t m_xAdvance;
  // yAdvance is used only in vertical text layouts.
};

// TODO(AB): Move to a separate file?
struct TextMetrics
{
  int32_t m_lineWidthInPixels {0};
  int32_t m_maxLineHeightInPixels {0};
  std::vector<GlyphMetrics> m_glyphs;

  void AddGlyphMetrics(int16_t font, uint16_t glyphId, int32_t xOffset, int32_t yOffset, int32_t xAdvance, int32_t height)
  {
    m_glyphs.push_back({font, glyphId, xOffset, yOffset, xAdvance});
    m_lineWidthInPixels += xAdvance;
    m_maxLineHeightInPixels = std::max(m_maxLineHeightInPixels, height);
  }
};
}  // namespace text

class GlyphManager
{
public:
  struct Params
  {
    std::string m_uniBlocks;
    std::string m_whitelist;
    std::string m_blacklist;

    std::vector<std::string> m_fonts;
  };

  explicit GlyphManager(Params const & params);
  ~GlyphManager();

  Glyph GetGlyph(strings::UniChar unicodePoints);

  void MarkGlyphReady(Glyph const & glyph);
  bool AreGlyphsReady(strings::UniString const & str) const;

  Glyph const & GetInvalidGlyph() const;

  int GetFontIndex(strings::UniChar unicodePoint);
  int GetFontIndex(std::u16string_view sv);

  text::TextMetrics ShapeText(std::string_view utf8, int fontPixelHeight, int8_t lang);
  text::TextMetrics ShapeText(std::string_view utf8, int fontPixelHeight, char const * lang);

  GlyphImage GetGlyphImage(int fontIndex, uint16_t glyphId, int pixelHeight, bool sdf);

private:
  // Immutable version can be called from any thread and doesn't require internal synchronization.
  int GetFontIndexImmutable(strings::UniChar unicodePoint) const;
  int FindFontIndexInBlock(UnicodeBlock const & block, strings::UniChar unicodePoint) const;

  struct Impl;
  std::unique_ptr<Impl> m_impl;
};
}  // namespace dp
