#pragma once

#include "../std/shared_ptr.hpp"
#include "color.hpp"

namespace yg
{
  struct GlyphMetrics
  {
    int m_xAdvance;
    int m_yAdvance;
    int m_xOffset;
    int m_yOffset;
    int m_width;
    int m_height;
  };

  struct GlyphInfo
  {
    GlyphMetrics m_metrics;
    yg::Color m_color;
    vector<unsigned char> m_bitmap;

    void dump(char const * fileName);
  };

  struct GlyphKey
  {
    int m_id;
    int m_fontSize;
    bool m_isMask;
    uint32_t toUInt32() const;
    GlyphKey(int id, int fontSize, bool isMask);
  };

  bool operator<(GlyphKey const & l, GlyphKey const & r);

  struct GlyphCacheImpl;

  class GlyphCache
  {
  private:

    shared_ptr<GlyphCacheImpl> m_impl;

  public:

    GlyphCache(size_t maxSize);

    void reset();
    void addFont(char const * fileName);

    int getCharIDX(GlyphKey const & key);

    shared_ptr<GlyphInfo> const getGlyph(GlyphKey const & key);
    /// return control box(could be slightly larger than the precise bound box).
    GlyphMetrics const getGlyphMetrics(GlyphKey const & key);
  };
}
