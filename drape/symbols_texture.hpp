#pragma once

#include "texture.hpp"
#include "texture_structure_desc.hpp"

#include "../std/string.hpp"

class SymbolsTexture : public Texture
{
public:
  SymbolsTexture(string const & skinPathName);

  m2::RectD FindSymbol(string const & symbolName);

private:
  TextureStructureDesc m_desc;
};
