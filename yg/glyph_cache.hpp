#pragma once

#include "../std/shared_ptr.hpp"

namespace yg
{
  struct GlyphInfo
  {
    int m_xAdvance;
    int m_xOffset;
    int m_yOffset;
    int m_width;
    int m_height;

    vector<unsigned char> m_bitmap;

    void dump(char const * fileName);
  };

  struct GlyphKey
  {
    unsigned short m_id;
    int m_fontSize;
    bool m_isMask;
    GlyphKey(unsigned short id, int fontSize, bool isMask);
  };

  bool operator<(GlyphKey const & l, GlyphKey const & r);

  class GlyphCache
  {
  private:

    struct Impl;
    shared_ptr<Impl> m_impl;

  public:

    GlyphCache();
    ~GlyphCache();

    void addFont(char const * fileName);

    shared_ptr<GlyphInfo> const getGlyph(GlyphKey const & key);
  };
}
