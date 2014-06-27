#include "symbols_texture.hpp"

//#include "utils/lodepng.h"
#include "utils/stb_image.h"

#include "../platform/platform.hpp"

SymbolsTexture::SymbolKey::SymbolKey(string const & symbolName)
  : m_symbolName(symbolName)
{
}

Texture::Key::Type SymbolsTexture::SymbolKey::GetType() const
{
  return Texture::Key::Symbol;
}

string const & SymbolsTexture::SymbolKey::GetSymbolName() const
{
  return m_symbolName;
}

void SymbolsTexture::Load(string const & skinPathName)
{
  uint32_t width, height;
  vector<unsigned char> rawData;

  try
  {
    m_desc.Load(skinPathName + ".sdf", width, height);
    ReaderPtr<ModelReader> reader = GetPlatform().GetReader(skinPathName + ".png");
    uint64_t size = reader.Size();
    rawData.resize(size);
    reader.Read(0, &rawData[0], size);
  }
  catch (RootException & e)
  {
    LOG(LERROR, (e.what()));
    int32_t alfaTexture = 0;
    Create(1, 1, Texture::RGBA8, MakeStackRefPointer(&alfaTexture));
    return;
  }

  int w, h, bpp;
  unsigned char * data = stbi_png_load_from_memory(&rawData[0], rawData.size(), &w, &h, &bpp, 0);

  ASSERT(width == w && height == h, ());
  Create(width, height, Texture::RGBA8, MakeStackRefPointer<void>(data));
  delete [] data;
}

bool SymbolsTexture::FindResource(Texture::Key const & key, m2::RectF & texRect, m2::PointU & pixelSize) const
{
  if (key.GetType() != Texture::Key::Symbol)
    return false;

  string const & symbolName = static_cast<SymbolKey const &>(key).GetSymbolName();

  m2::RectU r;
  if (!m_desc.GetResource(symbolName, r))
    return false;

  pixelSize.x = r.SizeX();
  pixelSize.y = r.SizeY();
  texRect = m2::RectF(GetS(r.minX()), GetT(r.minY()), GetS(r.maxX()), GetT(r.maxY()));
  return true;
}
