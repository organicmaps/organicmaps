#pragma once

#include "texture.hpp"

#include "../std/string.hpp"
#include "../std/map.hpp"

class SymbolsTexture : public Texture
{
public:
  class SymbolKey : public Key
  {
  public:
    SymbolKey(string const & symbolName);
    virtual ResourceType GetType() const;
    string const & GetSymbolName() const;

  private:
    string m_symbolName;
  };

  class SymbolInfo : public ResourceInfo
  {
  public:
    SymbolInfo(m2::RectF const & texRect);
    virtual ResourceType GetType() const;
  };

  void Load(string const & skinPathName);
  ResourceInfo const * FindResource(Key const & key) const;

private:
  void Fail();

private:
  typedef map<string, SymbolInfo> tex_definition_t;
  tex_definition_t m_definition;

  class DefinitionLoader;
};
