#include "symbols_texture.hpp"

#include "utils/lodepng.h"

#include "../platform/platform.hpp"

SymbolsTexture::SymbolKey::SymbolKey(const string & symbolName)
  : m_symbolName(symbolName)
{
}

Texture::Key::Type SymbolsTexture::SymbolKey::GetType() const
{
  return Texture::Key::Symbol;
}

const string & SymbolsTexture::SymbolKey::GetSymbolName() const
{
  return m_symbolName;
}

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

bool SymbolsTexture::FindResource(const Texture::Key & key, m2::RectF & texRect, m2::PointU & pixelSize) const
{
  if (key.GetType() != Texture::Key::Symbol)
    return false;

  const string & symbolName = static_cast<const SymbolKey &>(key).GetSymbolName();

  m2::RectU r;
  if (!m_desc.GetResource(symbolName, r))
    return false;

  pixelSize.x = r.SizeX();
  pixelSize.y = r.SizeY();
  texRect = m2::RectF(GetS(r.minX()), GetT(r.minY()), GetS(r.maxX()), GetT(r.maxY()));
  return true;
}
