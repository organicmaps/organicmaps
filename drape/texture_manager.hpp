#pragma once

#include "symbols_texture.hpp"

class TextureManager
{
public:
  void LoadExternalResources(const string & resourcePostfix);

  struct Symbol
  {
    m2::RectD m_texCoord;
    uint32_t m_textureBlock;
  };

  void GetSymbolRect(const string & symbolName, Symbol & symbol) const;

private:
  SymbolsTexture m_symbols;
};
