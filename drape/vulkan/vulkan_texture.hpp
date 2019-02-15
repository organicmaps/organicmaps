#pragma once

#include "drape/hw_texture.hpp"
#include "drape/pointers.hpp"
#include "drape/vulkan/vulkan_object_manager.hpp"
#include "drape/vulkan/vulkan_utils.hpp"

#include <vulkan_wrapper.h>
#include <vulkan/vulkan.h>

#include <cstdint>

namespace dp
{
namespace vulkan
{
class VulkanStagingBuffer;

class VulkanTextureAllocator : public HWTextureAllocator
{
public:
  drape_ptr<HWTexture> CreateTexture(ref_ptr<dp::GraphicsContext> context) override;
  void Flush() override {}
};

class VulkanTexture : public HWTexture
{
  using Base = HWTexture;
public:
  explicit VulkanTexture(ref_ptr<VulkanTextureAllocator> allocator);
  ~VulkanTexture() override;

  void Create(ref_ptr<dp::GraphicsContext> context, Params const & params,
              ref_ptr<void> data) override;
  void UploadData(ref_ptr<dp::GraphicsContext> context, uint32_t x, uint32_t y, uint32_t width,
                  uint32_t height, ref_ptr<void> data) override;
  void Bind(ref_ptr<dp::GraphicsContext> context) const override;
  void SetFilter(TextureFilter filter) override;
  bool Validate() const override;

  VkImageView GetTextureView() const { return m_textureObject.m_imageView; }
  VkImage GetImage() const { return m_textureObject.m_image; }
  SamplerKey GetSamplerKey() const;
  
private:
  ref_ptr<VulkanTextureAllocator> m_allocator;
  ref_ptr<VulkanObjectManager> m_objectManager;
  VulkanObject m_textureObject;
  mutable drape_ptr<VulkanStagingBuffer> m_creationStagingBuffer;
  uint32_t m_reservationId = 0;
  bool m_isMutable = false;
};
}  // namespace vulkan
}  // namespace dp
