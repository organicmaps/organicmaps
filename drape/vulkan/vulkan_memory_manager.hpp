#pragma once

#include <vulkan_wrapper.h>
#include <vulkan/vulkan.h>

namespace dp
{
namespace vulkan
{
class VulkanMemoryManager
{
public:
  explicit VulkanMemoryManager(VkDevice device) : m_device(device) {}

  //VkDeviceMemory

private:
  VkDevice const m_device;
};
}  // namespace vulkan
}  // namespace dp
