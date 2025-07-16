#pragma once

#include "drape/pointers.hpp"

#include <vulkan_wrapper.h>

#include <array>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

namespace dp
{
namespace vulkan
{
// NOTE: The class is not thread safe and must be externally synchronized.
class VulkanMemoryManager
{
public:
  VulkanMemoryManager(VkDevice device, VkPhysicalDeviceLimits const & deviceLimits,
                      VkPhysicalDeviceMemoryProperties const & memoryProperties);
  ~VulkanMemoryManager();

  enum class ResourceType : uint8_t
  {
    Geometry = 0,
    Uniform,
    Staging,
    Image,

    Count
  };
  static size_t constexpr kResourcesCount = static_cast<uint32_t>(VulkanMemoryManager::ResourceType::Count);

  struct MemoryBlock
  {
    VkDeviceMemory m_memory = {};
    uint32_t m_blockSize = 0;
    uint32_t m_freeOffset = 0;
    uint32_t m_allocationCounter = 0;
    bool m_isCoherent = false;
    bool m_isBlocked = false;
  };

  struct Allocation
  {
    uint64_t const m_blockHash;
    uint32_t const m_alignedOffset;
    uint32_t const m_alignedSize;
    ResourceType const m_resourceType;
    ref_ptr<MemoryBlock> m_memoryBlock;

    Allocation(ResourceType resourceType, uint64_t blockHash, uint32_t offset, uint32_t size,
               ref_ptr<MemoryBlock> memoryBlock)
      : m_blockHash(blockHash)
      , m_alignedOffset(offset)
      , m_alignedSize(size)
      , m_resourceType(resourceType)
      , m_memoryBlock(memoryBlock)
    {}
  };

  using AllocationPtr = std::shared_ptr<Allocation>;

  AllocationPtr Allocate(ResourceType resourceType, VkMemoryRequirements memReqs, uint64_t blockHash);
  void BeginDeallocationSession();
  void Deallocate(AllocationPtr ptr);
  void EndDeallocationSession();

  uint32_t GetOffsetAlignment(ResourceType resourceType) const;
  uint32_t GetSizeAlignment(VkMemoryRequirements const & memReqs) const;
  static uint32_t GetAligned(uint32_t value, uint32_t alignment);

  VkPhysicalDeviceLimits const & GetDeviceLimits() const;

private:
  std::optional<uint32_t> GetMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties) const;
  void IncrementTotalAllocationsCount();
  void DecrementTotalAllocationsCount();

  VkDevice const m_device;
  VkPhysicalDeviceLimits const m_deviceLimits;
  VkPhysicalDeviceMemoryProperties const m_memoryProperties;
  bool m_isInDeallocationSession = false;
  uint32_t m_deallocationSessionMask = 0;
  uint32_t m_totalAllocationCounter = 0;

  using MemoryBlocks = std::vector<drape_ptr<MemoryBlock>>;
  std::array<std::unordered_map<uint64_t, MemoryBlocks>, kResourcesCount> m_memory;
  std::array<MemoryBlocks, kResourcesCount> m_freeBlocks;
  std::array<uint32_t, kResourcesCount> m_sizes = {};
};
}  // namespace vulkan
}  // namespace dp
