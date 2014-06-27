#pragma once

#include "../std/map.hpp"
#include "../base/string_utils.hpp"
#include "texture_font.hpp"
#include "../platform/platform.hpp"

#include "../base/stl_add.hpp"


using namespace std;

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
  void Add(FontChar & symbol);
  int GetBlockByUnicode(int unicode);
  bool GetSymbolByUnicode(int unicode, FontChar & symbol);
  vector<TextureFont> Load(string const & path);
};
