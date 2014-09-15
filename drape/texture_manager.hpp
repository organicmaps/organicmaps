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
  virtual void GetStippleRegion(StipplePenKey const & pen, StippleRegion & region) const;
  virtual void GetColorRegion(ColorKey const & pen, TextureSetHolder::ColorRegion & region) const;
  virtual int GetMaxTextureSet() const;

  virtual void UpdateDynamicTextures();

  void BindTextureSet(uint32_t textureSet) const;
  uint32_t GetTextureCount(uint32_t textureSet) const;

private:
  template <typename TKey, typename TRegion>
  bool FindResource(TKey const & key, TRegion & region) const;

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
