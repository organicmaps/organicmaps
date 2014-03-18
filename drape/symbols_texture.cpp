#include "symbols_texture.hpp"

#include "utils/lodepng.h"

SymbolsTexture::SymbolsTexture(const string & skinPathName)
{
  uint32_t width, height;
  m_desc.Load(skinPathName + ".sdf", width, height);

  vector<unsigned char> pngData;
  unsigned w, h;
  lodepng::decode(pngData, w, h, skinPathName + ".png");

  ASSERT(width == w && height == h, ());
  Create(width, height, Texture::RGBA8, MakeStackRefPointer<void>(&pngData[0]));
}

m2::RectD SymbolsTexture::FindSymbol(const string & symbolName)
{
  m2::RectU r;
  if (!m_desc.GetResource(symbolName, r))
    return m2::RectD();

  return m2::RectD(GetS(r.minX()), GetT(r.minY()), GetS(r.maxX()), GetT(r.maxY()));
}
