#pragma once

#include "base/buffer_vector.hpp"
#include "base/shared_buffer_manager.hpp"

#include <cstdint>
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
  GlyphImage(GlyphImage const &) = delete;
  GlyphImage & operator=(GlyphImage const &) = delete;
  GlyphImage(GlyphImage &&) noexcept = default;
  GlyphImage & operator=(GlyphImage &&) noexcept = default;

  // m_data's deleter returns the underlying buffer to SharedBufferManager's pool.

  uint32_t m_width = 0;
  uint32_t m_height = 0;

  SharedBufferManager::shared_buffer_ptr_t m_data;
};

struct GlyphFontAndId
{
  int16_t m_fontIndex = 0;
  uint16_t m_glyphId = 0;

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

// Perfect 32-bit hash: the two 16-bit fields are packed without collisions across the full
// (fontIndex, glyphId) key space. m_fontIndex is widened through uint16_t first so a negative
// value (e.g. kInvalidFont == -1) doesn't sign-extend into the upper bits before the shift.
struct GlyphFontAndIdHash
{
  size_t operator()(GlyphFontAndId const & k) const noexcept
  {
    return (static_cast<uint32_t>(static_cast<uint16_t>(k.m_fontIndex)) << 16) | static_cast<uint32_t>(k.m_glyphId);
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
