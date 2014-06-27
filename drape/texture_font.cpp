#include "texture_font.hpp"

bool TextureFont::FindResource(Texture::Key const & key, m2::RectF & texRect, m2::PointU & pixelSize) const
{
  if (key.GetType() != Texture::Key::Font)
    return false;

  map<int, FontChar>::const_iterator itm = m_chars.find(static_cast<FontKey const &>(key).GetUnicode());
  if(itm == m_chars.end())
    return false;

  FontChar const & symbol = itm->second;

  pixelSize.x = symbol.m_width;
  pixelSize.y = symbol.m_height;

  float x_start = (float)symbol.m_x / (float)GetWidth();
  float y_start = (float)symbol.m_y / (float)GetHeight();
  float dx = (float)symbol.m_width / (float)GetWidth();
  float dy = (float)symbol.m_height / (float)GetHeight();

  texRect = m2::RectF(x_start, y_start, x_start + dx, y_start + dy);

  return true;
}

bool TextureFont::GetSymbolByUnicode(int unicode, FontChar & symbol) const
{
  map<int, FontChar>::const_iterator itm = m_chars.find(unicode);
  if(itm == m_chars.end())
    return false;

  symbol = itm->second;
  return true;
}

void TextureFont::Load(int size, void *data, int32_t blockNum)
{
  m_blockNum = blockNum;
  Create(size, size, Texture::ALPHA, MakeStackRefPointer<void>(data));
}

void TextureFont::Add(FontChar const & symbol)
{
  m_chars.insert(make_pair(symbol.m_unicode, symbol));
}
