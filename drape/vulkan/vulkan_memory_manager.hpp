#pragma once

#include <vulkan_wrapper.h>
#include <vulkan/vulkan.h>

#include <boost/optional.hpp>

#include <array>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>
#include <unordered_map>

namespace dp
{
namespace vulkan
{
// NOTE: The class is not thread safe and must be externally synchronized.
class VulkanMemoryManager
{
public:
  VulkanMemoryManager(VkDevice device, VkPhysicalDeviceLimits const & deviceLimits,
                      VkPhysicalDeviceMemoryProperties const & memoryProperties)
    : m_device(device)
    , m_deviceLimits(deviceLimits)
    , m_memoryProperties(memoryProperties)
  {}

  ~VulkanMemoryManager();

  enum class ResourceType : uint8_t
  {
    Geometry = 0,
    Uniform,
    Staging,
    Image,

    Count
  };
  static size_t constexpr kResourcesCount =
      static_cast<uint32_t>(VulkanMemoryManager::ResourceType::Count);

  struct Allocation
  {
    uint64_t const m_blockHash;
    VkDeviceMemory const m_memory;
    uint32_t const m_offset;
    uint32_t const m_size;
    ResourceType const m_resourceType;
    bool const m_isCoherent;

    Allocation(ResourceType resourceType, uint64_t blockHash, VkDeviceMemory memory,
               uint32_t offset, uint32_t size, bool isCoherent)
      : m_blockHash(blockHash)
      , m_memory(memory)
      , m_offset(offset)
      , m_size(size)
      , m_resourceType(resourceType)
      , m_isCoherent(isCoherent)
    {}
  };

  using AllocationPtr = std::shared_ptr<Allocation>;

  AllocationPtr Allocate(ResourceType resourceType, VkMemoryRequirements memReqs,
                         uint64_t blockHash);
  void BeginDeallocationSession();
  void Deallocate(AllocationPtr ptr);
  void EndDeallocationSession();

private:
  boost::optional<uint32_t> GetMemoryTypeIndex(uint32_t typeBits,
                                               VkMemoryPropertyFlags properties) const;
  uint32_t GetOffsetAlignment(ResourceType resourceType) const;

  VkDevice const m_device;
  VkPhysicalDeviceLimits const m_deviceLimits;
  VkPhysicalDeviceMemoryProperties const m_memoryProperties;
  bool m_isInDeallocationSession = false;
  uint32_t m_deallocationSessionMask = 0;

  struct MemoryBlock
  {
    VkDeviceMemory m_memory = {};
    uint32_t m_blockSize = 0;
    uint32_t m_freeOffset = 0;
    uint32_t m_allocationCounter = 0;
    bool m_isCoherent = false;

    bool operator<(MemoryBlock const & b) const { return m_blockSize < b.m_blockSize; }
  };

  std::array<std::unordered_map<uint64_t, std::vector<MemoryBlock>>, kResourcesCount> m_memory;
  std::array<std::vector<MemoryBlock>, kResourcesCount> m_freeBlocks;
  std::array<uint32_t, kResourcesCount> m_sizes = {};
};
}  // namespace vulkan
}  // namespace dp
