#include "font_loader.hpp"

#include "../platform/platform.hpp"

#include "glfunctions.hpp"
#include "glconstants.hpp"
#include "utils/lodepng.h"

#include "../std/string.hpp"
#include "../std/map.hpp"
#include "../std/vector.hpp"

FontLoader::FontLoader()
{
}

void FontLoader::Add(FontChar &symbol)
{
  symbol.m_blockNum = (symbol.m_y / m_supportedSize) * m_blockCnt + symbol.m_x / m_supportedSize;
  symbol.m_x %= m_supportedSize;
  symbol.m_y %= m_supportedSize;
  m_dictionary.insert(pair<int, FontChar>(symbol.m_unicode, symbol));
}

int FontLoader::GetBlockByUnicode(int unicode)
{
  return m_dictionary[unicode].m_blockNum;
}

FontChar FontLoader::GetSymbolByUnicode(int unicode)
{
  return m_dictionary[unicode];
}

vector<TextureFont> FontLoader::Load(string const & path)
{
  m_dictionary.clear();
  vector<unsigned char> image, buffer;
  unsigned w, h;

  try
  {
    ReaderPtr<ModelReader> reader = GetPlatform().GetReader(path + ".png");
    uint64_t size = reader.Size();
    buffer.resize(size);
    reader.Read(0, &buffer[0], size);
  }
  catch (RootException & e)
  {
    LOG(LERROR, (e.what()));
    return vector<TextureFont>();
  }

  lodepng::decode(image, w, h, &buffer[0], buffer.size(), LCT_GREY);

  m_realSize = w;
  int32_t maxTextureSize = GLFunctions::glGetInteger(gl_const::GLMaxTextureSize);
  m_supportedSize = min(m_realSize, maxTextureSize);
  m_blockCnt = m_realSize / m_supportedSize;

  vector<TextureFont> pages;
  pages.resize(m_blockCnt * m_blockCnt);

  buffer.resize(m_supportedSize * m_supportedSize);
  for(int i = 0 ; i < m_blockCnt ; ++i)
  {
    for(int j = 0; j < m_blockCnt ; ++j)
    {
      vector<unsigned char>::iterator itr_x = image.begin() + i * m_realSize * m_supportedSize + j * m_supportedSize;
      for(int k = 0; k < m_supportedSize ; k++)
      {
        for(int l = 0; l < m_supportedSize ; l++)
        {
          buffer[k * m_supportedSize + l] = (*itr_x);
          itr_x ++;
        }
        itr_x += (m_realSize - m_supportedSize);
      }
      pages[i * m_blockCnt + j].Load(m_supportedSize, buffer.data(), i * m_blockCnt + j);
    }
  }

  string s;
  try
  {
    ReaderPtr<ModelReader> reader = GetPlatform().GetReader(path + ".txt");
    reader.ReadAsString(s);
  }
  catch (RootException & e)
  {
    LOG(LERROR, (e.what()));
    return pages;
  }

  int pos = s.find("\n");
  FontChar symbol;
  while(pos != string::npos)
  {
    string line = s.substr(0, pos);
    symbol.ReadFromString(line);
    Add(symbol);
    pages[symbol.m_blockNum].Add(symbol);
    s = s.substr(pos+1, s.length());
    pos = s.find("\n");
  }
  if(s.compare("") != 0)
  {
    symbol.ReadFromString(s);
    Add(symbol);
    pages[symbol.m_blockNum].Add(symbol);
  }

  return pages;
}
