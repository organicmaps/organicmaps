#include "font_loader.hpp"

#include "../platform/platform.hpp"

#include "glfunctions.hpp"
#include "glconstants.hpp"
#include "utils/stb_image.h"

#include "../std/string.hpp"
#include "../std/map.hpp"
#include "../std/vector.hpp"

#include <boost/gil/image_view.hpp>
#include <boost/gil/algorithm.hpp>
#include <boost/gil/typedefs.hpp>

using boost::gil::gray8_view_t;
using boost::gil::gray8c_view_t;
using boost::gil::interleaved_view;
using boost::gil::subimage_view;
using boost::gil::copy_pixels;
using boost::gil::gray8c_pixel_t;
using boost::gil::gray8_pixel_t;

namespace
{
  void CreateFontChar(string & s, FontChar & symbol)
  {
    vector<string> tokens;
    strings::Tokenize(s, "\t", MakeBackInsertFunctor(tokens));

    strings::to_int(tokens[0].c_str(), symbol.m_unicode);
    strings::to_int(tokens[1].c_str(), symbol.m_x);
    strings::to_int(tokens[2].c_str(), symbol.m_y);
    strings::to_int(tokens[3].c_str(), symbol.m_width);
    strings::to_int(tokens[4].c_str(), symbol.m_height);
    strings::to_int(tokens[5].c_str(), symbol.m_xOffset);
    strings::to_int(tokens[6].c_str(), symbol.m_yOffset);
    strings::to_int(tokens[7].c_str(), symbol.m_spacing);
  }
}

void FontLoader::Add(FontChar & symbol)
{
  symbol.m_blockNum = (symbol.m_y / m_supportedSize) * m_blockCnt + symbol.m_x / m_supportedSize;
  symbol.m_x %= m_supportedSize;
  symbol.m_y %= m_supportedSize;
  m_dictionary.insert(make_pair(symbol.m_unicode, symbol));
}

int FontLoader::GetBlockByUnicode(int unicode)
{
  FontChar symbol;
  if(GetSymbolByUnicode(unicode, symbol))
    return symbol.m_blockNum;

  return -1;
}

bool FontLoader::GetSymbolByUnicode(int unicode, FontChar & symbol)
{
  map<int, FontChar>::const_iterator itm = m_dictionary.find(unicode);
  if(itm == m_dictionary.end())
    return false;

  symbol = itm->second;
  return true;
}

vector<TextureFont> FontLoader::Load(string const & path)
{
  m_dictionary.clear();
  vector<unsigned char> buffer;

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

  int w, h, bpp;
  unsigned char * data = stbi_png_load_from_memory(&buffer[0], buffer.size(), &w, &h, &bpp, 0);

  m_realSize = w;
  int32_t maxTextureSize = GLFunctions::glGetInteger(gl_const::GLMaxTextureSize);
  m_supportedSize = min(m_realSize, maxTextureSize);
  m_blockCnt = m_realSize / m_supportedSize;

  vector<TextureFont> pages;
  pages.resize(m_blockCnt * m_blockCnt);

  buffer.resize(m_supportedSize * m_supportedSize);
  gray8c_view_t srcImage = interleaved_view(w, h, (gray8c_pixel_t const *)data, w);

  for(int i = 0 ; i < m_blockCnt ; ++i)
  {
    for(int j = 0; j < m_blockCnt ; ++j)
    {
      gray8c_view_t subSrcImage = subimage_view(srcImage, j * m_supportedSize, i * m_supportedSize, m_supportedSize, m_supportedSize);
      gray8_view_t dstImage = interleaved_view(m_supportedSize, m_supportedSize, (gray8_pixel_t *)&buffer[0], m_supportedSize);
      copy_pixels(subSrcImage, dstImage);
      pages[i * m_blockCnt + j].Load(m_supportedSize, &buffer[0], i * m_blockCnt + j);
    }
  }
  delete [] data;
  buffer.clear();

  string s;
  try
  {
    ReaderPtr<ModelReader> reader = GetPlatform().GetReader(path + ".txt");
    reader.ReadAsString(s);
  }
  catch (RootException & e)
  {
    LOG(LERROR, (e.what()));
    pages.clear();
    return vector<TextureFont>();
  }

  vector<string> tokens;
  strings::Tokenize(s, "\n", MakeBackInsertFunctor(tokens));

  for(int i = 0; i < tokens.size() ; ++i)
  {
    FontChar symbol;
    CreateFontChar(tokens[i], symbol);
    Add(symbol);
    pages[symbol.m_blockNum].Add(symbol);
  }

  return pages;
}
