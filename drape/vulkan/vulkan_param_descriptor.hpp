#pragma once

#include "drape/graphics_context.hpp"
#include "drape/vulkan/vulkan_gpu_program.hpp"

#include <vulkan_wrapper.h>
#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>
#include <vector>

namespace dp
{
namespace vulkan
{
struct ParamDescriptor
{
  enum class Type : uint8_t
  {
    DynamicUniformBuffer,
    Texture
  };

  Type m_type = Type::DynamicUniformBuffer;

  VkDescriptorBufferInfo m_bufferDescriptor = {};
  uint32_t m_bufferDynamicOffset = 0;

  VkDescriptorImageInfo m_imageDescriptor = {};
  int8_t m_textureSlot = 0;
};

struct DescriptorSetGroup
{
  VkDescriptorSet m_descriptorSet = {};
  VkDescriptorPool m_descriptorPool = {};

  explicit operator bool()
  {
    return m_descriptorSet != VK_NULL_HANDLE &&
           m_descriptorPool != VK_NULL_HANDLE;
  }

  void Update(VkDevice device, std::vector<ParamDescriptor> const & descriptors);
};

class VulkanObjectManager;

class ParamDescriptorUpdater
{
public:
  explicit ParamDescriptorUpdater(ref_ptr<VulkanObjectManager> objectManager);

  void Update(ref_ptr<dp::GraphicsContext> context);
  void Reset();
  VkDescriptorSet GetDescriptorSet() const;

private:
  ref_ptr<VulkanObjectManager> m_objectManager;
  std::vector<DescriptorSetGroup> m_descriptorSetGroups;
  ref_ptr<VulkanGpuProgram> m_program;
  uint32_t m_updateDescriptorFrame = 0;
  uint32_t m_descriptorSetIndex = 0;
};

}  // namespace vulkan
}  // namespace dp
