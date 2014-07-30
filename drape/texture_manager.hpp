#pragma once

#include "../base/string_utils.hpp"

#include "pointers.hpp"
#include "texture_set_holder.hpp"
#include "texture_set_controller.hpp"

namespace dp
{

class TextureManager : public TextureSetHolder
{
public:
  void Init(string const & resourcePrefix);
  void Release();
  virtual void GetSymbolRegion(string const & symbolName, SymbolRegion & region) const;
  virtual bool GetGlyphRegion(strings::UniChar charCode, GlyphRegion & region) const;
  virtual int GetMaxTextureSet() const;

  void BindTextureSet(uint32_t textureSet) const;
  uint32_t GetTextureCount(uint32_t textureSet) const;

private:
  class TextureSet;
  vector<MasterPointer<TextureSet> > m_textures;
  uint32_t m_maxTextureBlocks;
};

class TextureSetBinder : public TextureSetController
{
public:
  TextureSetBinder(RefPointer<TextureManager> manager);
  void BindTextureSet(uint32_t textureSet) const;
  uint32_t GetTextureCount(uint32_t textureSet) const;

private:
  RefPointer<TextureManager> m_manager;
};

} // namespace dp
