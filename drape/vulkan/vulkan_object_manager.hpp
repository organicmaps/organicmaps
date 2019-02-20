#pragma once

#include "drape/pointers.hpp"
#include "drape/vulkan/vulkan_gpu_program.hpp"
#include "drape/vulkan/vulkan_memory_manager.hpp"
#include "drape/vulkan/vulkan_utils.hpp"

#include "base/assert.hpp"

#include <vulkan_wrapper.h>
#include <vulkan/vulkan.h>

#include <cstdint>
#include <map>
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

  VkDeviceMemory GetMemory() const
  {
    ASSERT(m_allocation != nullptr, ());
    ASSERT(m_allocation->m_memoryBlock != nullptr, ());
    return m_allocation->m_memoryBlock->m_memory;
  }

  uint32_t GetAlignedOffset() const
  {
    ASSERT(m_allocation != nullptr, ());
    return m_allocation->m_alignedOffset;
  }

  uint32_t GetAlignedSize() const
  {
    ASSERT(m_allocation != nullptr, ());
    return m_allocation->m_alignedSize;
  }
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
  VulkanObject CreateImage(VkImageUsageFlags usageFlags, VkFormat format,
                           VkImageAspectFlags aspectFlags, uint32_t width, uint32_t height);
  DescriptorSetGroup CreateDescriptorSetGroup(ref_ptr<VulkanGpuProgram> program,
                                              std::vector<ParamDescriptor> const & descriptors);

  // Use unsafe function ONLY if an object exists on the only thread, otherwise
  // use safe Fill function.
  uint8_t * MapUnsafe(VulkanObject object);
  void FlushUnsafe(VulkanObject object, uint32_t offset = 0, uint32_t size = 0);
  void UnmapUnsafe(VulkanObject object);
  void Fill(VulkanObject object, void const * data, uint32_t sizeInBytes);

  void DestroyObject(VulkanObject object);
  void DestroyDescriptorSetGroup(DescriptorSetGroup group);
  void CollectObjectsSync();
  void CollectObjectsAsync();

  VkDevice GetDevice() const { return m_device; }
  VulkanMemoryManager const & GetMemoryManager() const { return m_memoryManager; };
  VkSampler GetSampler(SamplerKey const & key);

private:
  void CreateDescriptorPool();
  void DestroyDescriptorPools();

  VkDevice const m_device;
  uint32_t const m_queueFamilyIndex;
  VulkanMemoryManager m_memoryManager;
  std::vector<VulkanObject> m_queueToDestroy;

  std::vector<VkDescriptorPool> m_descriptorPools;
  std::vector<DescriptorSetGroup> m_descriptorsToDestroy;

  std::map<SamplerKey, VkSampler> m_samplers;

  std::mutex m_mutex;
  std::mutex m_destroyMutex;
};
}  // namespace vulkan
}  // namespace dp
