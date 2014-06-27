#pragma once

#include "texture_font.hpp"

#include "../platform/platform.hpp"

#include "../base/string_utils.hpp"
#include "../base/stl_add.hpp"

#include "../std/map.hpp"

struct FontChar
{
public:
  int m_unicode;
  int m_x;
  int m_y;
  int m_height;
  int m_width;
  int m_xOffset;
  int m_yOffset;
  int m_spacing;
  int m_blockNum;
};

class TextureFont;
class FontLoader
{
private:
  int m_realSize;
  int m_supportedSize;
  int m_blockCnt;

  map<int, FontChar> m_dictionary;

public:
  void Add(FontChar const & symbol);
  int GetSymbolCoords(FontChar & symbol) const;
  int GetBlockByUnicode(int unicode) const;
  bool GetSymbolByUnicode(int unicode, FontChar & symbol) const;
  vector<TextureFont> Load(string const & path);
};
