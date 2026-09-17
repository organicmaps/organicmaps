#include "drape/vulkan/vulkan_memory_manager.hpp"
#include "drape/vulkan/vulkan_utils.hpp"

#include "base/assert.hpp"
#include "base/math.hpp"

#include <algorithm>
#include <limits>

namespace dp
{
namespace vulkan
{
namespace
{
// Only shared geometry blocks profit from a minimal size, an exclusive block holds a single allocation.
std::array<uint32_t, VulkanMemoryManager::kResourcesCount> const kMinBlockSizeInBytes = {{
    1024 * 1024,  // Geometry
    0,            // Uniform
    0,            // Storage
    0,            // Staging
    0,            // Image
}};

std::array<uint32_t, VulkanMemoryManager::kResourcesCount> const kDesiredSizeInBytes = {{
    80 * 1024 * 1024,                      // Geometry
    std::numeric_limits<uint32_t>::max(),  // Uniform (unlimited)
    std::numeric_limits<uint32_t>::max(),  // Storage (unlimited)
    20 * 1024 * 1024,                      // Staging
    100 * 1024 * 1024,                     // Image
}};

// Uniform, storage and staging buffers are mapped outside of the VulkanObjectManager lock, for an upload or for
// their whole lifetime, so they never share a memory block: a VkDeviceMemory can't be mapped twice, and another
// thread may deallocate a neighbour allocation while the block is mapped.
std::array<bool, VulkanMemoryManager::kResourcesCount> const kExclusiveBlocks = {{
    false,  // Geometry (mapped only under the lock)
    true,   // Uniform
    true,   // Storage
    true,   // Staging
    false,  // Image (never mapped)
}};

VkMemoryPropertyFlags GetMemoryPropertyFlags(VulkanMemoryManager::ResourceType resourceType,
                                             std::optional<VkMemoryPropertyFlags> & fallbackTypeBits)
{
  switch (resourceType)
  {
  case VulkanMemoryManager::ResourceType::Geometry:
    fallbackTypeBits = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

  case VulkanMemoryManager::ResourceType::Staging:
    // No fallback.
    return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

  case VulkanMemoryManager::ResourceType::Uniform:
    // No fallback.
    return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

  case VulkanMemoryManager::ResourceType::Storage:
    // No fallback.
    return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

  case VulkanMemoryManager::ResourceType::Image:
    // No fallback.
    return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

  case VulkanMemoryManager::ResourceType::Count: CHECK(false, ());
  }
  return 0;
}

struct LessBlockSize
{
  using BlockPtrT = drape_ptr<VulkanMemoryManager::MemoryBlock>;

  bool operator()(BlockPtrT const & b1, BlockPtrT const & b2) const { return b1->m_blockSize < b2->m_blockSize; }
  bool operator()(BlockPtrT const & b1, uint32_t b2) const { return b1->m_blockSize < b2; }
  bool operator()(uint32_t b1, BlockPtrT const & b2) const { return b1 < b2->m_blockSize; }
};
}  // namespace

VulkanMemoryManager::VulkanMemoryManager(VkDevice device, VkPhysicalDeviceLimits const & deviceLimits,
                                         VkPhysicalDeviceMemoryProperties const & memoryProperties)
  : m_device(device)
  , m_deviceLimits(deviceLimits)
  , m_memoryProperties(memoryProperties)
  , m_uniformAlignment(math::LCM(static_cast<uint32_t>(m_deviceLimits.minUniformBufferOffsetAlignment),
                                 static_cast<uint32_t>(m_deviceLimits.nonCoherentAtomSize)))
  , m_storageAlignment(math::LCM(static_cast<uint32_t>(m_deviceLimits.minStorageBufferOffsetAlignment),
                                 static_cast<uint32_t>(m_deviceLimits.nonCoherentAtomSize)))
  , m_baseAlignment(math::LCM(static_cast<uint32_t>(m_deviceLimits.minMemoryMapAlignment),
                              static_cast<uint32_t>(m_deviceLimits.nonCoherentAtomSize)))
{}

VulkanMemoryManager::~VulkanMemoryManager()
{
  for (size_t i = 0; i < kResourcesCount; ++i)
  {
    for (auto const & b : m_freeBlocks[i])
    {
      DecrementTotalAllocationsCount();
      vkFreeMemory(m_device, b->m_memory, nullptr);
    }

    for (auto const & p : m_memory[i])
    {
      for (auto const & b : p.second)
      {
        DecrementTotalAllocationsCount();
        vkFreeMemory(m_device, b->m_memory, nullptr);
      }
    }
  }
  ASSERT_EQUAL(m_totalAllocationCounter, 0, ());
}

std::optional<uint32_t> VulkanMemoryManager::GetMemoryTypeIndex(uint32_t typeBits,
                                                                VkMemoryPropertyFlags properties) const
{
  for (uint32_t i = 0; i < m_memoryProperties.memoryTypeCount; i++)
  {
    if ((typeBits & 1) == 1)
    {
      if ((m_memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
        return i;
    }
    typeBits >>= 1;
  }
  return {};
}

uint32_t VulkanMemoryManager::GetOffsetAlignment(ResourceType resourceType) const
{
  if (resourceType == ResourceType::Uniform)
    return m_uniformAlignment;

  if (resourceType == ResourceType::Storage)
    return m_storageAlignment;

  return m_baseAlignment;
}

uint32_t VulkanMemoryManager::GetSizeAlignment(VkMemoryRequirements const & memReqs) const
{
  return math::LCM(static_cast<uint32_t>(memReqs.alignment), static_cast<uint32_t>(m_deviceLimits.nonCoherentAtomSize));
}

// static
uint32_t VulkanMemoryManager::GetAligned(uint32_t value, uint32_t alignment)
{
  if (alignment == 0)
    return value;
  return (value + alignment - 1) & ~(alignment - 1);
}

VulkanMemoryManager::AllocationPtr VulkanMemoryManager::Allocate(ResourceType resourceType,
                                                                 VkMemoryRequirements memReqs, uint64_t blockHash)
{
  size_t const intResType = static_cast<size_t>(resourceType);
  auto const alignedSize = GetAligned(static_cast<uint32_t>(memReqs.size), GetSizeAlignment(memReqs));
  auto & m = m_memory[intResType];
  // Looking for an existed block.
  if (!kExclusiveBlocks[intResType])
  {
    auto const it = m.find(blockHash);
    if (it != m.end())
    {
      CHECK(!it->second.empty(), ());
      auto & block = it->second.back();
      auto const alignedOffset = GetAligned(block->m_freeOffset, GetOffsetAlignment(resourceType));

      // Shared blocks are mapped only under the lock, see kExclusiveBlocks.
      ASSERT(!block->m_isBlocked, ());
      // There is space in the current block.
      if (block->m_blockSize >= alignedOffset + alignedSize)
      {
        block->m_freeOffset = alignedOffset + alignedSize;
        block->m_allocationCounter++;
        return std::make_shared<Allocation>(resourceType, blockHash, alignedOffset, alignedSize, make_ref(block));
      }
    }
  }

  // Looking for a block in free ones.
  {
    auto & fm = m_freeBlocks[intResType];
    // Free blocks array must be sorted by size.
    auto const freeBlockIt = std::lower_bound(fm.begin(), fm.end(), alignedSize, LessBlockSize());
    if (freeBlockIt != fm.end())
    {
      drape_ptr<MemoryBlock> freeBlock = std::move(*freeBlockIt);
      CHECK_EQUAL(freeBlock->m_allocationCounter, 0, ());
      CHECK_EQUAL(freeBlock->m_freeOffset, 0, ());
      CHECK_LESS_OR_EQUAL(alignedSize, freeBlock->m_blockSize, ());
      CHECK(!freeBlock->m_isBlocked, ());
      fm.erase(freeBlockIt);

      freeBlock->m_freeOffset = alignedSize;
      freeBlock->m_allocationCounter++;
      auto p = std::make_shared<Allocation>(resourceType, blockHash, 0, alignedSize, make_ref(freeBlock));

      m[blockHash].push_back(std::move(freeBlock));
      return p;
    }
  }

  // Looking for memory index by memory properties.
  std::optional<VkMemoryPropertyFlags> fallbackFlags;
  auto flags = GetMemoryPropertyFlags(resourceType, fallbackFlags);
  auto memoryTypeIndex = GetMemoryTypeIndex(memReqs.memoryTypeBits, flags);
  if (!memoryTypeIndex && fallbackFlags)
  {
    flags = *fallbackFlags;
    memoryTypeIndex = GetMemoryTypeIndex(memReqs.memoryTypeBits, flags);
  }

  CHECK(memoryTypeIndex, ("Unsupported memory allocation configuration."));

  // Create new memory block.
  auto const blockSize = std::max(kMinBlockSizeInBytes[intResType], alignedSize);
  VkDeviceMemory memory = {};
  VkMemoryAllocateInfo memAllocInfo = {};
  memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  memAllocInfo.pNext = nullptr;
  memAllocInfo.allocationSize = blockSize;
  memAllocInfo.memoryTypeIndex = *memoryTypeIndex;
  IncrementTotalAllocationsCount();

  CHECK_VK_CALL_EX(vkAllocateMemory(m_device, &memAllocInfo, nullptr, &memory),
                   ("Requested size =", blockSize, "Allocated sizes =", m_sizes, "Total allocs =",
                    m_totalAllocationCounter, m_memory[intResType].size(), m_freeBlocks[intResType].size()));

  m_sizes[intResType] += blockSize;

  // Attach block.
  auto newBlock = make_unique_dp<MemoryBlock>();
  newBlock->m_memory = memory;
  newBlock->m_blockSize = blockSize;
  newBlock->m_freeOffset = alignedSize;
  newBlock->m_allocationCounter++;
  newBlock->m_isCoherent = ((flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0);

  auto p = std::make_shared<Allocation>(resourceType, blockHash, 0, alignedSize, make_ref(newBlock));
  m[blockHash].push_back(std::move(newBlock));
  return p;
}

void VulkanMemoryManager::BeginDeallocationSession()
{
  m_isInDeallocationSession = true;
  m_deallocationSessionMask = 0;
}

void VulkanMemoryManager::Deallocate(AllocationPtr ptr)
{
  CHECK(ptr, ());
  CHECK(m_isInDeallocationSession, ());
  CHECK(!ptr->m_memoryBlock->m_isBlocked, ());
  auto const resourceIndex = static_cast<size_t>(ptr->m_resourceType);
  auto & m = m_memory[resourceIndex];
  auto const it = m.find(ptr->m_blockHash);
  CHECK(it != m.end(), ());
  auto blockIt = std::find_if(it->second.begin(), it->second.end(), [&ptr](drape_ptr<MemoryBlock> const & b)
  {
    ASSERT(ptr->m_memoryBlock != nullptr, ());
    return b->m_memory == ptr->m_memoryBlock->m_memory;
  });
  CHECK(blockIt != it->second.end(), ());
  CHECK_GREATER((*blockIt)->m_allocationCounter, 0, ());
  (*blockIt)->m_allocationCounter--;

  // Set a bit in the deallocation mask to skip the processing of untouched resource collections.
  if ((*blockIt)->m_allocationCounter == 0)
    m_deallocationSessionMask |= (1 << resourceIndex);
}

void VulkanMemoryManager::EndDeallocationSession()
{
  if (!m_isInDeallocationSession)
    return;

  m_isInDeallocationSession = false;

  for (size_t i = 0; i < kResourcesCount; ++i)
  {
    if (((m_deallocationSessionMask >> i) & 1) == 0)
      continue;

    auto & fm = m_freeBlocks[i];
    for (auto it = m_memory[i].begin(); it != m_memory[i].end();)
    {
      auto & m = it->second;
      m.erase(std::remove_if(m.begin(), m.end(),
                             [this, &fm, i](drape_ptr<MemoryBlock> & b)
      {
        if (b->m_allocationCounter != 0)
          return false;

        if (m_sizes[i] > kDesiredSizeInBytes[i])
        {
          CHECK_LESS_OR_EQUAL(b->m_blockSize, m_sizes[i], ());
          m_sizes[i] -= b->m_blockSize;
          DecrementTotalAllocationsCount();
          vkFreeMemory(m_device, b->m_memory, nullptr);
        }
        else
        {
          auto block = std::move(b);
          block->m_freeOffset = 0;
          fm.push_back(std::move(block));
        }
        return true;
      }),
              m.end());

      // Mirror correspondent CHECK(!it->second.empty()) in VulkanMemoryManager::Allocate.
      if (m.empty())
        it = m_memory[i].erase(it);
      else
        ++it;
    }

    std::sort(fm.begin(), fm.end(), LessBlockSize());
  }
}

void VulkanMemoryManager::IncrementTotalAllocationsCount()
{
  ++m_totalAllocationCounter;
  CHECK_LESS_OR_EQUAL(m_totalAllocationCounter, m_deviceLimits.maxMemoryAllocationCount, ());
}

void VulkanMemoryManager::DecrementTotalAllocationsCount()
{
  CHECK_GREATER(m_totalAllocationCounter, 0, ());
  --m_totalAllocationCounter;
}

VkPhysicalDeviceLimits const & VulkanMemoryManager::GetDeviceLimits() const
{
  return m_deviceLimits;
}
}  // namespace vulkan
}  // namespace dp
