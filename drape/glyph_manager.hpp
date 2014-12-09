#pragma once

#include "../base/shared_buffer_manager.hpp"
#include "../base/string_utils.hpp"

#include "../std/unique_ptr.hpp"
#include "../std/string.hpp"
#include "../std/vector.hpp"

namespace dp
{

class GlyphManager
{
public:
  struct Params
  {
    string m_uniBlocks;
    string m_whitelist;
    string m_blacklist;

    vector<string> m_fonts;

    uint32_t m_baseGlyphHeight = 20;
  };

  struct GlyphMetrics
  {
    int m_xAdvance;
    int m_yAdvance;
    int m_xOffset;
    int m_yOffset;
    int m_width;
    int m_height;
  };

  struct GlyphImage
  {
    ~GlyphImage()
    {
      ASSERT(!m_data.unique(), ());
    }

    void Destroy()
    {
      SharedBufferManager::instance().freeSharedBuffer(m_bufferSize, m_data);
    }

    int m_width;
    int m_height;

    SharedBufferManager::shared_buffer_ptr_t m_data;
    size_t m_bufferSize;
  };

  struct Glyph
  {
    GlyphMetrics m_metrics;
    GlyphImage m_image;
  };

  GlyphManager(Params const & params);
  ~GlyphManager();

  void GetGlyphs(vector<strings::UniChar> const & unicodePoints, vector<Glyph> & glyphs);

private:
  Glyph const & GetInvalidGlyph() const;

private:
  struct Impl;
  Impl * m_impl;
};

}
