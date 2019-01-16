#pragma once

#include "drape/hw_texture.hpp"
#include "drape/pointers.hpp"
#include "drape/vulkan/vulkan_device_holder.hpp"

#include <vulkan_wrapper.h>
#include <vulkan/vulkan.h>

#include <cstdint>

namespace dp
{
namespace vulkan
{
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

  void Create(ref_ptr<dp::GraphicsContext> context, Params const & params, ref_ptr<void> data) override;
  void UploadData(uint32_t x, uint32_t y, uint32_t width, uint32_t height, ref_ptr<void> data) override;
  void Bind() const override {}
  void SetFilter(TextureFilter filter) override;
  bool Validate() const override;

  VkImageView GetTextureView() const { return m_textureView; }
  
private:
  ref_ptr<VulkanTextureAllocator> m_allocator;
  DeviceHolderPtr m_deviceHolder;
  VkImage m_texture = {};
  VkImageView m_textureView = {};
  bool m_isMutable = false;
};
}  // namespace vulkan
}  // namespace dp
