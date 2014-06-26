#pragma once

#include <fstream>
#include <map>
#include "texture_font.h"
#include "../platform/platform.hpp"

using namespace std;

class FontChar
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

public:
  FontChar(){}
  ~FontChar(){}

  void read(ifstream &in)
  {
    in>>m_unicode>>m_x>>m_y>>m_width>>m_height>>m_xOffset>>m_yOffset>>m_spacing;
  }

  void ReadFromString(string & s)
  {
    vector<string> tokens;
    int pos;
    pos = s.find("\t");
    while(pos != string::npos)
    {
      tokens.push_back(s.substr(0, pos));
      s = s.substr(pos+1, s.length());
      pos = s.find("\t");
    }
    tokens.push_back(s);

    m_unicode = atoi(tokens[0].c_str());
    m_x = atoi(tokens[1].c_str());
    m_y = atoi(tokens[2].c_str());
    m_width = atoi(tokens[3].c_str());
    m_height = atoi(tokens[4].c_str());
    m_xOffset = atoi(tokens[5].c_str());
    m_yOffset = atoi(tokens[6].c_str());
    m_spacing = atoi(tokens[7].c_str());
  }
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
  FontLoader();
  void Add(FontChar &symbol);
  int GetBlockByUnicode(int unicode);
  FontChar GetSymbolByUnicode(int unicode);
  vector<TextureFont> Load(const string &path);
};
