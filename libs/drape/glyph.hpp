#pragma once

#include "base/assert.hpp"
#include "base/buffer_vector.hpp"
#include "base/shared_buffer_manager.hpp"

#include <tuple>  // std::tie

namespace dp
{
struct GlyphImage
{
  GlyphImage() = default;
  GlyphImage(uint32_t w, uint32_t h, SharedBufferManager::shared_buffer_ptr_t d)
    : m_width(w)
    , m_height(h)
    , m_data(std::move(d))
  {}
  ~GlyphImage() { ASSERT(!m_data, ("Probably you forgot to call Destroy()")); }
  GlyphImage(GlyphImage const &) = delete;
  GlyphImage & operator=(GlyphImage const &) = delete;
  GlyphImage(GlyphImage &&) noexcept = default;
  GlyphImage & operator=(GlyphImage &&) noexcept = default;

  void Destroy()
  {
    if (m_data)
    {
      auto const sz = m_data->size();
      SharedBufferManager::Instance().FreeSharedBuffer(sz, std::move(m_data));
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
  Glyph(GlyphImage && image, GlyphFontAndId key) : m_image(std::move(image)), m_key(key) {}

  GlyphImage m_image;
  GlyphFontAndId m_key;
};
}  // namespace dp
