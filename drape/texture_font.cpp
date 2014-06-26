#include "texture_font.h"

TextureFont::TextureFont()
{
}

bool TextureFont::FindResource(Texture::Key const & key, m2::RectF & texRect, m2::PointU & pixelSize) const
{
  if (key.GetType() != Texture::Key::Font)
    return false;

  FontKey K = static_cast<FontKey const &>(key);

  map<int, FontChar>::iterator itm = m_chars.find(K.GetUnicode());
  if(itm == m_chars.end())
    return false;

  FontChar symbol = itm->second;

  pixelSize.x = symbol.m_width;
  pixelSize.y = symbol.m_height;

  float x_start = (float)symbol.m_x / (float)GetWidth();
  float y_start = (float)symbol.m_y / (float)GetHeight();
  float dx = (float)symbol.m_width / (float)GetWidth();
  float dy = (float)symbol.m_height / (float)GetHeight();

  texRect = m2::RectF(x_start, y_start, x_start + dx, y_start + dy);

  return true;
}

FontChar TextureFont::GetSymbolByUnicode(int unicode)
{
  return m_chars[unicode];
}

void TextureFont::Load(int size, void *data, int32_t blockNum)
{
  m_blockNum = blockNum;
  Create(size, size, Texture::ALPHA, MakeStackRefPointer<void>(data));
}

void TextureFont::Add(FontChar symbol)
{
  m_chars.insert(pair<int, FontChar>(symbol.m_unicode, symbol));
}
