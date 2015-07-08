#pragma once

#include "drape/texture.hpp"

#include "std/string.hpp"
#include "std/map.hpp"

namespace dp
{

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

  explicit SymbolsTexture(string const & skinPathName);

  ref_ptr<ResourceInfo> FindResource(Key const & key, bool & newResource) override;

  void Invalidate(string const & skinPathName);

private:
  void Fail();
  void Load(string const & skinPathName);

  typedef map<string, SymbolInfo> TSymDefinition;
  mutable TSymDefinition m_definition;

  class DefinitionLoader;
};

} // namespace dp
