#pragma once

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

  struct StagingData
  {
    VkBuffer m_stagingBuffer = {};
    uint32_t m_offset = 0;
    operator bool() const { return m_stagingBuffer != 0; }
  };
  struct StagingPointer
  {
    StagingData m_stagingData;
    uint8_t * m_pointer = nullptr;
  };
  StagingPointer GetDefaultStagingBuffer(uint32_t sizeInBytes);
  StagingData CopyWithDefaultStagingBuffer(uint32_t sizeInBytes, void * data);
  void FlushDefaultStagingBuffer();
  void ResetDefaultStagingBuffer();

  // The result buffer will be destroyed after the nearest command queue submitting.
  StagingData CopyWithTemporaryStagingBuffer(uint32_t sizeInBytes, void * data);

  void DestroyObject(VulkanObject object);
  void CollectObjects();

  VulkanMemoryManager const & GetMemoryManager() const { return m_memoryManager; };

private:
  VkDevice const m_device;
  uint32_t const m_queueFamilyIndex;
  VulkanMemoryManager m_memoryManager;
  std::vector<VulkanObject> m_queueToDestroy;

  VulkanObject m_defaultStagingBuffer;
  uint8_t * m_defaultStagingBufferPtr = nullptr;
  uint32_t m_defaultStagingBufferAlignment = 0;
  uint32_t m_defaultStagingBufferOffset = 0;

  std::mutex m_mutex;
};
}  // namespace vulkan
}  // namespace dp
