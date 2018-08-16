#pragma once

#include "drape/texture.hpp"

#include <string>

namespace dp
{
class StaticTexture : public Texture
{
  using Base = Texture;
public:
  class StaticKey : public Key
  {
  public:
    ResourceType GetType() const override { return ResourceType::Static; }
  };

  static std::string const kDefaultResource;

  StaticTexture(std::string const & textureName, std::string const & skinPathName,
                dp::TextureFormat format, ref_ptr<HWTextureAllocator> allocator);

  ref_ptr<ResourceInfo> FindResource(Key const & key, bool & newResource) override;
  void Create(Params const & params) override;
  void Create(Params const & params, ref_ptr<void> data) override;

  void Invalidate(ref_ptr<HWTextureAllocator> allocator);

  bool IsLoadingCorrect() const { return m_isLoadingCorrect; }
private:
  void Fail();
  bool Load(ref_ptr<HWTextureAllocator> allocator);

  std::string const m_textureName;
  std::string const m_skinPathName;
  dp::TextureFormat const m_format;

  drape_ptr<Texture::ResourceInfo> m_info;

  bool m_isLoadingCorrect;
};
}  // namespace dp
