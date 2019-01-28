#pragma once

#include "drape/pointers.hpp"
#include "drape/vulkan/vulkan_memory_manager.hpp"

#include <vulkan_wrapper.h>
#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

namespace dp
{
namespace vulkan
{
struct VulkanObject
{
  VkBuffer m_buffer = {};
  VkImage m_image = {};
  VkImageView m_imageView = {};
  VulkanMemoryManager::AllocationPtr m_allocation;
};

class VulkanStagingBuffer;

class VulkanObjectManager
{
public:
  VulkanObjectManager(VkDevice device, VkPhysicalDeviceLimits const & deviceLimits,
                      VkPhysicalDeviceMemoryProperties const & memoryProperties,
                      uint32_t queueFamilyIndex);
  ~VulkanObjectManager();

  VulkanObject CreateBuffer(VulkanMemoryManager::ResourceType resourceType,
                            uint32_t sizeInBytes, uint64_t batcherHash);
  VulkanObject CreateImage(VkImageUsageFlagBits usageFlagBits, VkFormat format,
                           VkImageAspectFlags aspectFlags, uint32_t width, uint32_t height);


  ref_ptr<VulkanStagingBuffer> GetDefaultStagingBuffer() const;
  void FlushDefaultStagingBuffer();
  void ResetDefaultStagingBuffer();

  void DestroyObject(VulkanObject object);
  void CollectObjects();

  VkDevice GetDevice() const { return m_device; }
  VulkanMemoryManager const & GetMemoryManager() const { return m_memoryManager; };

private:
  VkDevice const m_device;
  uint32_t const m_queueFamilyIndex;
  VulkanMemoryManager m_memoryManager;
  std::vector<VulkanObject> m_queueToDestroy;

  drape_ptr<VulkanStagingBuffer> m_defaultStagingBuffer;

  std::mutex m_mutex;
};
}  // namespace vulkan
}  // namespace dp
