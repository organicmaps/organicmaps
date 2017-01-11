#pragma once

#include "drape/texture.hpp"

#include "std/string.hpp"

namespace dp
{

class StaticTexture : public Texture
{
public:
  class StaticKey : public Key
  {
  public:
    ResourceType GetType() const override { return ResourceType::Static; }
  };

  StaticTexture(string const & textureName, string const & skinPathName,
                ref_ptr<HWTextureAllocator> allocator);

  ref_ptr<ResourceInfo> FindResource(Key const & key, bool & newResource) override;

  void Invalidate(string const & skinPathName, ref_ptr<HWTextureAllocator> allocator);

  bool IsLoadingCorrect() const { return m_isLoadingCorrect; }

private:
  void Fail();
  bool Load(string const & skinPathName, ref_ptr<HWTextureAllocator> allocator);

  string m_textureName;
  drape_ptr<Texture::ResourceInfo> m_info;

  bool m_isLoadingCorrect;
};

} // namespace dp
