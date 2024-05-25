#pragma once

#include "base/assert.hpp"
#include "base/shared_buffer_manager.hpp"
#include "base/string_utils.hpp"

namespace dp
{
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
    ASSERT_NOT_EQUAL(m_data.use_count(), 1, ("Probably you forgot to call Destroy()"));
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

  SharedBufferManager::shared_buffer_ptr_t m_data;
};

struct Glyph
{
  GlyphMetrics m_metrics;
  GlyphImage m_image;
  int m_fontIndex;
  strings::UniChar m_code;
};
}  // namespace dp

