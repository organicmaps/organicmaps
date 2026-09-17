#include "testing/testing.hpp"

#include "drape/vulkan/vulkan_memory_manager.hpp"

#include <vulkan_wrapper.h>

#include <cstdint>

namespace vulkan_memory_manager_tests
{
using dp::vulkan::VulkanMemoryManager;
using ResourceType = VulkanMemoryManager::ResourceType;

uint32_t constexpr kKB = 1024;
uint32_t constexpr kMB = 1024 * kKB;

// Vulkan entry points are function pointers in vulkan_wrapper, so the allocator is testable without a device.
uint64_t g_lastMemoryHandle = 0;

VKAPI_ATTR VkResult VKAPI_CALL FakeAllocateMemory(VkDevice, VkMemoryAllocateInfo const *, VkAllocationCallbacks const *,
                                                  VkDeviceMemory * memory)
{
  ++g_lastMemoryHandle;
#if VK_USE_64_BIT_PTR_DEFINES
  *memory = reinterpret_cast<VkDeviceMemory>(g_lastMemoryHandle);
#else
  *memory = g_lastMemoryHandle;
#endif
  return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL FakeFreeMemory(VkDevice, VkDeviceMemory, VkAllocationCallbacks const *) {}

VulkanMemoryManager CreateManager()
{
  vkAllocateMemory = &FakeAllocateMemory;
  vkFreeMemory = &FakeFreeMemory;

  VkPhysicalDeviceLimits limits = {};
  limits.minUniformBufferOffsetAlignment = 256;
  limits.minStorageBufferOffsetAlignment = 256;
  limits.minMemoryMapAlignment = 64;
  limits.nonCoherentAtomSize = 64;
  limits.maxMemoryAllocationCount = 4096;

  VkPhysicalDeviceMemoryProperties memoryProperties = {};
  memoryProperties.memoryTypeCount = 1;
  memoryProperties.memoryTypes[0].propertyFlags =
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  memoryProperties.memoryHeapCount = 1;

  return VulkanMemoryManager(VK_NULL_HANDLE, limits, memoryProperties);
}

VkMemoryRequirements Requirements(uint32_t size)
{
  VkMemoryRequirements memReqs = {};
  memReqs.size = size;
  memReqs.alignment = 64;
  memReqs.memoryTypeBits = 1;
  return memReqs;
}

void Deallocate(VulkanMemoryManager & mm, VulkanMemoryManager::AllocationPtr const & allocation)
{
  mm.BeginDeallocationSession();
  mm.Deallocate(allocation);
  mm.EndDeallocationSession();
}

UNIT_TEST(VulkanMemoryManager_GeometryBlocksAreShared)
{
  auto mm = CreateManager();
  uint64_t constexpr kBatcherHash = 1;

  auto const a1 = mm.Allocate(ResourceType::Geometry, Requirements(100 * kKB), kBatcherHash);
  auto const a2 = mm.Allocate(ResourceType::Geometry, Requirements(100 * kKB), kBatcherHash);
  TEST(a1->m_memoryBlock->m_memory == a2->m_memoryBlock->m_memory, ());
  TEST_EQUAL(a1->m_alignedOffset, 0, ());
  TEST_GREATER_OR_EQUAL(a2->m_alignedOffset, a1->m_alignedSize, ());

  Deallocate(mm, a1);
  Deallocate(mm, a2);
}

UNIT_TEST(VulkanMemoryManager_MappedBuffersDoNotShareBlocks)
{
  auto mm = CreateManager();

  // A released staging block is kept for reuse.
  auto const big = mm.Allocate(ResourceType::Staging, Requirements(4 * kMB), 0 /* blockHash */);
  auto const bigMemory = big->m_memoryBlock->m_memory;
  Deallocate(mm, big);

  // A smaller staging buffer takes the whole block, nothing is packed into the leftover space.
  auto const s1 = mm.Allocate(ResourceType::Staging, Requirements(1 * kMB), 0 /* blockHash */);
  TEST(s1->m_memoryBlock->m_memory == bigMemory, ());
  auto const s2 = mm.Allocate(ResourceType::Staging, Requirements(1 * kMB), 0 /* blockHash */);
  TEST(s2->m_memoryBlock->m_memory != bigMemory, ());
  TEST_EQUAL(s2->m_alignedOffset, 0, ());

  // Uniform and storage blocks are not shared either.
  for (auto const type : {ResourceType::Uniform, ResourceType::Storage})
  {
    auto const b1 = mm.Allocate(type, Requirements(16 * kKB), 0 /* blockHash */);
    auto const b2 = mm.Allocate(type, Requirements(16 * kKB), 0 /* blockHash */);
    TEST(b1->m_memoryBlock->m_memory != b2->m_memoryBlock->m_memory, ());
    Deallocate(mm, b2);
    Deallocate(mm, b1);
  }

  Deallocate(mm, s2);
  Deallocate(mm, s1);
}

// A texture staging buffer is allocated into a reused staging block, then a temporary staging buffer of the
// frontend renderer is allocated and mapped without the object manager lock, then the first one is collected on a
// DrapeRoutine thread while the second one is still mapped.
UNIT_TEST(VulkanMemoryManager_DeallocateNextToMappedStagingBuffer)
{
  auto mm = CreateManager();

  auto const big = mm.Allocate(ResourceType::Staging, Requirements(4 * kMB), 0 /* blockHash */);
  Deallocate(mm, big);

  auto const collected = mm.Allocate(ResourceType::Staging, Requirements(1 * kMB), 0 /* blockHash */);
  auto const mapped = mm.Allocate(ResourceType::Staging, Requirements(1 * kMB), 0 /* blockHash */);
  mapped->m_memoryBlock->m_isBlocked = true;

  Deallocate(mm, collected);

  mapped->m_memoryBlock->m_isBlocked = false;
  Deallocate(mm, mapped);
}
}  // namespace vulkan_memory_manager_tests
