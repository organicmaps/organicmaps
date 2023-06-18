#pragma once

#include "drape/texture.hpp"

#include <optional>
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

  StaticTexture(ref_ptr<dp::GraphicsContext> context,
                std::string const & textureName,
                std::optional<std::string> const & skinPathName,
                dp::TextureFormat format,
                ref_ptr<HWTextureAllocator> allocator,
                bool allowOptional = false);

  ref_ptr<ResourceInfo> FindResource(Key const & key, bool & newResource) override;
  void Create(ref_ptr<dp::GraphicsContext> context, Params const & params) override;
  void Create(ref_ptr<dp::GraphicsContext> context, Params const & params,
              ref_ptr<void> data) override;

  void Invalidate(ref_ptr<dp::GraphicsContext> context, ref_ptr<HWTextureAllocator> allocator,
                  std::vector<drape_ptr<HWTexture>> & internalTextures);
  void Invalidate(ref_ptr<dp::GraphicsContext> context, ref_ptr<HWTextureAllocator> allocator);

  bool IsLoadingCorrect() const { return m_isLoadingCorrect; }
  
  // Invalidation is required after updating texture name to reload the texture.
  void UpdateTextureName(std::string const & textureName,
                         std::optional<std::string> const & skinPathName);
  
private:
  void Fail(ref_ptr<dp::GraphicsContext> context);
  bool Load(ref_ptr<dp::GraphicsContext> context, ref_ptr<HWTextureAllocator> allocator);

  std::string m_textureName;
  std::optional<std::string> m_skinPathName;
  
  dp::TextureFormat const m_format;
  bool const m_allowOptional;

  drape_ptr<Texture::ResourceInfo> m_info;

  bool m_isLoadingCorrect;
};
}  // namespace dp
