#pragma once

#include "texture.hpp"
#include "texture_structure_desc.hpp"

#include "../std/string.hpp"

class SymbolsTexture : public Texture
{
public:
  class SymbolKey : public Key
  {
  public:
    SymbolKey(string const & symbolName);
    virtual Type GetType() const;
    string const & GetSymbolName() const;

  private:
    string m_symbolName;
  };

  void Load(string const & skinPathName);
  bool FindResource(Key const & key, m2::RectF & texRect, m2::PointU & pixelSize) const;

private:
  TextureStructureDesc m_desc;
};
