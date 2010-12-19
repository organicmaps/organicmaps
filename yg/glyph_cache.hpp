#pragma once

#include "../std/shared_ptr.hpp"
#include "color.hpp"

namespace yg
{
  struct GlyphInfo
  {
    int m_xAdvance;
    int m_xOffset;
    int m_yOffset;
    int m_width;
    int m_height;
    yg::Color m_color;

    vector<unsigned char> m_bitmap;

    void dump(char const * fileName);
  };

  struct GlyphKey
  {
    int m_id;
    int m_fontSize;
    bool m_isMask;
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

    shared_ptr<GlyphInfo> const getGlyph(GlyphKey const & key);
  };
}
