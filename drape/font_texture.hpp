#pragma once

#include "pointers.hpp"
#include "texture.hpp"

#include "../std/map.hpp"
#include "../std/vector.hpp"
#include "../std/string.hpp"

class FontTexture : public Texture
{
public:
  class GlyphKey : public Key
  {
  public:
    GlyphKey(int32_t unicode) : m_unicode(unicode) {}

    ResourceType GetType() const { return Texture::Glyph; }
    int32_t GetUnicodePoint() const { return m_unicode; }

  private:
    int32_t m_unicode;
  };

  class GlyphInfo : public ResourceInfo
  {
  public:
    GlyphInfo(m2::RectF const & texRect, float xOffset,
              float yOffset, float advance);

    virtual ResourceType GetType() const { return Texture::Glyph; }
    void GetMetrics(float & xOffset, float & yOffset, float & advance) const;

  private:
    float m_xOffset, m_yOffset;
    float m_advance;
  };

public:
  ResourceInfo const * FindResource(Key const & key) const;

  void Add(int unicodePoint, GlyphInfo const & glyphInfo);

private:
  typedef map<int, GlyphInfo> glyph_map_t;
  glyph_map_t m_glyphs;
};

void LoadFont(string const & resourcePrefix, vector<TransferPointer<Texture> > & textures);
