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

private:
  void Fail();
  void Load(string const & skinPathName, ref_ptr<HWTextureAllocator> allocator);

  string m_textureName;
  drape_ptr<Texture::ResourceInfo> m_info;
};

} // namespace dp
