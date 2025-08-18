#pragma once

#include "drape/glyph.hpp"

#include "base/string_utils.hpp"

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
  GlyphFontAndId m_key;

  // TODO(AB): Store original font units or floats?
  int32_t m_xOffset;
  int32_t m_yOffset;
  int32_t m_xAdvance;
  // yAdvance is used only in vertical text layouts and is 0 for horizontal texts.
  int32_t m_yAdvance{0};

  bool operator==(GlyphMetrics const & other) const { return m_key == other.m_key; }
};

// TODO(AB): Move to a separate file?
struct TextMetrics
{
  int32_t m_lineWidthInPixels{0};
  int32_t m_maxLineHeightInPixels{0};
  std::vector<GlyphMetrics> m_glyphs;
  // Used for SplitText.
  bool m_isRTL{false};

  void AddGlyphMetrics(int16_t font, uint16_t glyphId, int32_t xOffset, int32_t yOffset, int32_t xAdvance,
                       int32_t height)
  {
    m_glyphs.push_back({{font, glyphId}, xOffset, yOffset, xAdvance});

    if (m_glyphs.size() == 1)
      xAdvance -= xOffset;
    // if (yOffset > 0)
    //   height += yOffset;  // TODO(AB): Is it needed? Is it correct?
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

  void MarkGlyphReady(GlyphFontAndId key);
  bool AreGlyphsReady(TGlyphs const & str) const;

  int GetFontIndex(strings::UniChar unicodePoint);
  int GetFontIndex(std::u16string_view sv);

  text::TextMetrics ShapeText(std::string_view utf8, int fontPixelHeight, int8_t lang);
  text::TextMetrics ShapeText(std::string_view utf8, int fontPixelHeight, char const * lang);

  GlyphImage GetGlyphImage(GlyphFontAndId key, int pixelHeight, bool sdf) const;

private:
  // Immutable version can be called from any thread and doesn't require internal synchronization.
  int GetFontIndexImmutable(strings::UniChar unicodePoint) const;
  int FindFontIndexInBlock(UnicodeBlock const & block, strings::UniChar unicodePoint) const;

  struct Impl;
  std::unique_ptr<Impl> m_impl;
};
}  // namespace dp
