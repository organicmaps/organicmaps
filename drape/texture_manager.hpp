#pragma once

#include "pointers.hpp"
#include "texture_set_holder.hpp"
#include "texture_set_controller.hpp"

class TextureManager : public TextureSetHolder
{
public:
  void Init(const string & resourcePrefix);
  void Release();
  void GetSymbolRegion(const string & symbolName, TextureRegion & region) const;

  void BindTextureSet(uint32_t textureSet) const;
  uint32_t GetTextureCount(uint32_t textureSet) const;

private:
  class TextureSet;
  MasterPointer<TextureSet> m_textures;
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
