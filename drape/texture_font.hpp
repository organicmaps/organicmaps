#pragma once

#include "texture.hpp"
#include "font_loader.hpp"
#include "../std/map.hpp"

using namespace std;

struct FontChar;
class TextureFont : public Texture
{
public:
  class FontKey : public Key
  {
  public:
    FontKey(int32_t unicode) : m_unicode(unicode) {}
    virtual Type GetType() const { return Texture::Key::Font; }
    int32_t GetUnicode() const { return m_unicode; }

  private:
    int32_t m_unicode;
  };

public:
  bool FindResource(Key const & key, m2::RectF & texRect, m2::PointU & pixelSize) const;
  void Load(int size, void * data, int32_t blockNum);
  void Add(FontChar const & letter);

  bool GetSymbolByUnicode(int unicode, FontChar & symbol) const;

private:
  map<int, FontChar> m_chars;
  int32_t m_blockNum;
};
