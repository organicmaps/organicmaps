#include "symbols_texture.hpp"

#include "utils/lodepng.h"

#include "../platform/platform.hpp"

void SymbolsTexture::Load(const string & skinPathName)
{
  uint32_t width, height;
  m_desc.Load(skinPathName + ".sdf", width, height);

  ReaderPtr<ModelReader> reader = GetPlatform().GetReader(skinPathName + ".png");
  uint64_t size = reader.Size();
  vector<unsigned char> rawData;
  rawData.resize(size);
  reader.Read(0, &rawData[0], size);

  vector<unsigned char> pngData;
  unsigned w, h;
  lodepng::decode(pngData, w, h, &rawData[0], rawData.size());

  ASSERT(width == w && height == h, ());
  Create(width, height, Texture::RGBA8, MakeStackRefPointer<void>(&pngData[0]));
}

m2::RectD SymbolsTexture::FindSymbol(const string & symbolName) const
{
  m2::RectU r;
  if (!m_desc.GetResource(symbolName, r))
    return m2::RectD();

  return m2::RectD(GetS(r.minX()), GetT(r.minY()), GetS(r.maxX()), GetT(r.maxY()));
}
