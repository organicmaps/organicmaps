#pragma once

#include "base/shared_buffer_manager.hpp"
#include "base/string_utils.hpp"

#include "std/unique_ptr.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

#include "std/function.hpp"

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
    string m_uniBlocks;
    string m_whitelist;
    string m_blacklist;

    vector<string> m_fonts;

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

  GlyphManager(Params const & params);
  ~GlyphManager();

  Glyph GetGlyph(strings::UniChar unicodePoints, int fixedHeight);
  Glyph GenerateGlyph(Glyph const & glyph) const;

  void MarkGlyphReady(Glyph const & glyph);
  bool AreGlyphsReady(strings::UniString const & str, int fixedSize) const;

  typedef function<void (strings::UniChar start, strings::UniChar end)> TUniBlockCallback;
  void ForEachUnicodeBlock(TUniBlockCallback const & fn) const;

  Glyph GetInvalidGlyph(int fixedSize) const;

  uint32_t GetBaseGlyphHeight() const;

private:
  int GetFontIndex(strings::UniChar unicodePoint);
  // Immutable version can be called from any thread and doesn't require internal synchronization.
  int GetFontIndexImmutable(strings::UniChar unicodePoint) const;
  int FindFontIndexInBlock(UnicodeBlock const & block, strings::UniChar unicodePoint) const;

private:
  struct Impl;
  Impl * m_impl;
};

}
