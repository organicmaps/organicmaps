#pragma once

#include "base/shared_buffer_manager.hpp"
#include "base/string_utils.hpp"

#include "std/unique_ptr.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

#include "std/function.hpp"

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
      SharedBufferManager::instance().freeSharedBuffer(m_data->size(), m_data);
    }

    int m_width;
    int m_height;

    SharedBufferManager::shared_buffer_ptr_t m_data;
  };

  struct Glyph
  {
    GlyphMetrics m_metrics;
    GlyphImage m_image;
  };

  GlyphManager(Params const & params);
  ~GlyphManager();

  Glyph GetGlyph(strings::UniChar unicodePoints);

  typedef function<void (strings::UniChar start, strings::UniChar end)> TUniBlockCallback;
  void ForEachUnicodeBlock(TUniBlockCallback const & fn) const;

private:
  Glyph GetInvalidGlyph() const;

private:
  struct Impl;
  Impl * m_impl;
};

}
