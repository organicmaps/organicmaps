#pragma once

#include "base/assert.hpp"
#include "base/buffer_vector.hpp"
#include "base/shared_buffer_manager.hpp"

#include <tuple>  // std::tie

namespace dp
{
struct GlyphImage
{
  ~GlyphImage() { ASSERT_NOT_EQUAL(m_data.use_count(), 1, ("Probably you forgot to call Destroy()")); }

  // TODO(AB): Get rid of manual call to Destroy.
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

  SharedBufferManager::shared_buffer_ptr_t m_data;
};

struct GlyphFontAndId
{
  int16_t m_fontIndex;
  uint16_t m_glyphId;

  // Required only for buffer_vector's internal T m_static[N];
  GlyphFontAndId() = default;

  constexpr GlyphFontAndId(int16_t fontIndex, uint16_t glyphId) : m_fontIndex(fontIndex), m_glyphId(glyphId) {}

  bool operator==(GlyphFontAndId const & other) const
  {
    return m_fontIndex == other.m_fontIndex && m_glyphId == other.m_glyphId;
  }

  bool operator<(GlyphFontAndId const & other) const
  {
    return std::tie(m_fontIndex, m_glyphId) < std::tie(other.m_fontIndex, other.m_glyphId);
  }
};

// 50 glyphs should fit most of the strings based on tests in Switzerland and China.
using TGlyphs = buffer_vector<GlyphFontAndId, 50>;

struct Glyph
{
  Glyph(GlyphImage && image, GlyphFontAndId key) : m_image(image), m_key(key) {}

  GlyphImage m_image;
  GlyphFontAndId m_key;
};
}  // namespace dp
