#pragma once

#include "base/shared_buffer_manager.hpp"
#include "base/string_utils.hpp"

#include <string>
#include <vector>
#include <functional>

namespace dp
{
uint32_t constexpr kSdfBorder = 4;

struct UnicodeBlock;

class GlyphManager
{
public:
  static const int kDynamicGlyphSize;

  struct Params
  {
    std::string m_uniBlocks;
    std::string m_whitelist;
    std::string m_blacklist;

    std::vector<std::string> m_fonts;

    uint32_t m_baseGlyphHeight = 22;
    uint32_t m_sdfScale = 4;
  };

  struct GlyphMetrics
  {
    float m_xAdvance;
    float m_yAdvance;
    float m_xOffset;
    float m_yOffset;
    bool m_isValid;
  };

  struct GlyphImage
  {
    ~GlyphImage()
    {
      ASSERT(!m_data.unique(), ("Probably you forgot to call Destroy()"));
    }

    void Destroy()
    {
      if (m_data != nullptr)
      {
        SharedBufferManager::instance().freeSharedBuffer(m_data->size(), m_data);
        m_data = nullptr;
      }
    }

    uint32_t m_width;
    uint32_t m_height;

    uint32_t m_bitmapRows;
    int m_bitmapPitch;

    SharedBufferManager::shared_buffer_ptr_t m_data;
  };

  struct Glyph
  {
    GlyphMetrics m_metrics;
    GlyphImage m_image;
    int m_fontIndex;
    strings::UniChar m_code;
    int m_fixedSize;
  };

  explicit GlyphManager(Params const & params);
  ~GlyphManager();

  Glyph GetGlyph(strings::UniChar unicodePoints, int fixedHeight);

  void MarkGlyphReady(Glyph const & glyph);
  bool AreGlyphsReady(strings::UniString const & str, int fixedSize) const;

  using TUniBlockCallback = std::function<void(strings::UniChar start, strings::UniChar end)>;
  void ForEachUnicodeBlock(TUniBlockCallback const & fn) const;

  Glyph GetInvalidGlyph(int fixedSize) const;

  uint32_t GetBaseGlyphHeight() const;
  uint32_t GetSdfScale() const;

  static Glyph GenerateGlyph(Glyph const & glyph, uint32_t sdfScale);

private:
  int GetFontIndex(strings::UniChar unicodePoint);
  // Immutable version can be called from any thread and doesn't require internal synchronization.
  int GetFontIndexImmutable(strings::UniChar unicodePoint) const;
  int FindFontIndexInBlock(UnicodeBlock const & block, strings::UniChar unicodePoint) const;

private:
  struct Impl;
  Impl * m_impl;
};
}  // namespace dp
