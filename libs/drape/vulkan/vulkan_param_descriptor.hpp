#pragma once

#include "drape/graphics_context.hpp"
#include "drape/vulkan/vulkan_gpu_program.hpp"
#include "drape/vulkan/vulkan_utils.hpp"

#include <array>
#include <cstdint>
#include <limits>
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
    DynamicUniformBuffer = 0,
    Texture
  };

  Type m_type = Type::DynamicUniformBuffer;

  VkDescriptorBufferInfo m_bufferDescriptor = {};
  uint32_t m_bufferDynamicOffset = 0;

  VkDescriptorImageInfo m_imageDescriptor = {};
  int8_t m_textureSlot = 0;

  uint32_t m_id = 0;
};

size_t constexpr kMaxDescriptorSets = 8;

struct DescriptorSetGroup
{
  VkDescriptorSet m_descriptorSet = {};
  uint32_t m_descriptorPoolIndex = std::numeric_limits<uint32_t>::max();

  std::array<uint32_t, kMaxDescriptorSets> m_ids = {};
  bool m_updated = false;

  explicit operator bool()
  {
    return m_descriptorSet != VK_NULL_HANDLE && m_descriptorPoolIndex != std::numeric_limits<uint32_t>::max();
  }

  void Update(VkDevice device, std::vector<ParamDescriptor> const & descriptors);
};

class VulkanObjectManager;

class ParamDescriptorUpdater
{
public:
  explicit ParamDescriptorUpdater(ref_ptr<VulkanObjectManager> objectManager);

  void Update(ref_ptr<dp::GraphicsContext> context);
  void Destroy();
  VkDescriptorSet GetDescriptorSet() const;

private:
  void Reset(uint32_t inflightFrameIndex);

  ref_ptr<VulkanObjectManager> m_objectManager;
  struct UpdateData
  {
    std::vector<DescriptorSetGroup> m_descriptorSetGroups;
    ref_ptr<VulkanGpuProgram> m_program;
    uint32_t m_updateDescriptorFrame = 0;
    uint32_t m_descriptorSetIndex = 0;
  };
  std::array<UpdateData, kMaxInflightFrames> m_updateData;
  uint32_t m_currentInflightFrameIndex = 0;
};

}  // namespace vulkan
}  // namespace dp
