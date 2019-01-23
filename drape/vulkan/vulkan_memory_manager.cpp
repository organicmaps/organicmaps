#include "drape/vulkan/vulkan_memory_manager.hpp"
#include "drape/vulkan/vulkan_utils.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <limits>

namespace dp
{
namespace vulkan
{
namespace
{
std::array<uint32_t, VulkanMemoryManager::kResourcesCount> const kMinBlockSizeInBytes =
{
  64 * 1024,  // Geometry
  4 * 1024,   // Uniform
  0,          // Staging
  0,          // Image
};

std::array<uint32_t, VulkanMemoryManager::kResourcesCount> const kDesiredSizeInBytes =
{
  50 * 1024 * 1024,                      // Geometry
  std::numeric_limits<uint32_t>::max(),  // Uniform (unlimited)
  10 * 1024 * 1024,                      // Staging
  std::numeric_limits<uint32_t>::max(),  // Image (unlimited)
};

VkMemoryPropertyFlags GetMemoryPropertyFlags(VulkanMemoryManager::ResourceType resourceType,
                                             boost::optional<VkMemoryPropertyFlags> & fallbackTypeBits)
{
  switch (resourceType)
  {
  case VulkanMemoryManager::ResourceType::Geometry:
  case VulkanMemoryManager::ResourceType::Staging:
    fallbackTypeBits = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

  case VulkanMemoryManager::ResourceType::Uniform:
    // No fallback.
    return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

  case VulkanMemoryManager::ResourceType::Image:
    // No fallback.
    return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

  case VulkanMemoryManager::ResourceType::Count:
    CHECK(false, ());
  }
  return 0;
}

uint32_t GetAligned(uint32_t value, uint32_t alignment)
{
  if (alignment == 0)
    return value;
  return (value + alignment - 1) & ~(alignment - 1);
}
}  // namespace

VulkanMemoryManager::~VulkanMemoryManager()
{
  for (size_t i = 0; i < kResourcesCount; ++i)
  {
    for (auto const & b : m_freeBlocks[i])
      vkFreeMemory(m_device, b.m_memory, nullptr);

    for (auto const & p : m_memory[i])
    {
      for (auto const & b : p.second)
        vkFreeMemory(m_device, b.m_memory, nullptr);
    }
  }
}

boost::optional<uint32_t> VulkanMemoryManager::GetMemoryTypeIndex(uint32_t typeBits,
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
    return static_cast<uint32_t>(m_deviceLimits.minUniformBufferOffsetAlignment);

  return static_cast<uint32_t>(m_deviceLimits.minMemoryMapAlignment);
}

VulkanMemoryManager::AllocationPtr VulkanMemoryManager::Allocate(ResourceType resourceType,
                                                                 VkMemoryRequirements memReqs,
                                                                 uint64_t blockHash)
{
  auto const alignedSize = GetAligned(static_cast<uint32_t>(memReqs.size),
                                      static_cast<uint32_t>(memReqs.alignment));
  // Looking for an existed block.
  {
    auto & m = m_memory[static_cast<size_t>(resourceType)];
    auto const it = m.find(blockHash);
    if (it != m.end())
    {
      CHECK(!it->second.empty(), ());
      auto & block = it->second.back();
      auto const alignedOffset = GetAligned(block.m_freeOffset, GetOffsetAlignment(resourceType));

      // There is space in the current block.
      if (block.m_blockSize <= alignedOffset + alignedSize)
      {
        block.m_freeOffset = alignedOffset + alignedSize;
        block.m_allocationCounter++;
        return std::make_shared<Allocation>(resourceType, blockHash, block.m_memory,
                                            alignedOffset, alignedSize, block.m_isCoherent);
      }
    }

    // Looking for a block in free ones.
    auto & fm = m_freeBlocks[static_cast<size_t>(resourceType)];
    // Free blocks array must be sorted by size.
    MemoryBlock refBlock;
    refBlock.m_blockSize = alignedSize;
    auto const freeBlockIt = std::upper_bound(fm.begin(), fm.end(), refBlock);
    if (freeBlockIt != fm.end())
    {
      MemoryBlock freeBlock = *freeBlockIt;
      CHECK_EQUAL(freeBlock.m_allocationCounter, 0, ());
      CHECK_EQUAL(freeBlock.m_freeOffset, 0, ());
      CHECK_LESS_OR_EQUAL(alignedSize, freeBlock.m_blockSize, ());
      fm.erase(freeBlockIt);

      freeBlock.m_freeOffset = alignedSize;
      freeBlock.m_allocationCounter++;
      auto p = std::make_shared<Allocation>(resourceType, blockHash, freeBlock.m_memory,
                                            0, alignedSize, freeBlock.m_isCoherent);
      m[blockHash].push_back(std::move(freeBlock));
      return p;
    }
  }

  // Looking for memory index by memory properties.
  boost::optional<VkMemoryPropertyFlags> fallbackFlags;
  auto flags = GetMemoryPropertyFlags(resourceType, fallbackFlags);
  auto memoryTypeIndex = GetMemoryTypeIndex(memReqs.memoryTypeBits, flags);
  if (!memoryTypeIndex && fallbackFlags)
  {
    flags = fallbackFlags.value();
    memoryTypeIndex = GetMemoryTypeIndex(memReqs.memoryTypeBits, flags);
    if (!memoryTypeIndex)
      CHECK(false, ("Unsupported memory allocation configuration."));
  }
  else
  {
    CHECK(false, ("Unsupported memory allocation configuration."));
  }

  // Create new memory block.
  auto const blockSize = std::max(kMinBlockSizeInBytes[static_cast<size_t>(resourceType)],
                                  alignedSize);
  VkDeviceMemory memory = {};
  VkMemoryAllocateInfo memAllocInfo = {};
  memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  memAllocInfo.pNext = nullptr;
  memAllocInfo.allocationSize = blockSize;
  memAllocInfo.memoryTypeIndex = memoryTypeIndex.value();
  CHECK_VK_CALL(vkAllocateMemory(m_device, &memAllocInfo, nullptr, &memory));
  m_sizes[static_cast<size_t>(resourceType)] += blockSize;

  // Attach block.
  auto & m = m_memory[static_cast<size_t>(resourceType)];

  MemoryBlock newBlock;
  newBlock.m_memory = memory;
  newBlock.m_blockSize = blockSize;
  newBlock.m_freeOffset = alignedSize;
  newBlock.m_allocationCounter++;
  newBlock.m_isCoherent = ((flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0);

  auto p = std::make_shared<Allocation>(resourceType, blockHash, newBlock.m_memory,
                                        0, alignedSize, newBlock.m_isCoherent);
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
  auto const kResourceIndex = static_cast<size_t>(ptr->m_resourceType);
  auto & m = m_memory[kResourceIndex];
  auto const it = m.find(ptr->m_blockHash);
  CHECK(it != m.end(), ());
  auto blockIt = std::find_if(it->second.begin(), it->second.end(),
                              [&ptr](MemoryBlock const & b)
  {
    return b.m_memory == ptr->m_memory;
  });
  CHECK(blockIt != it->second.end(), ());
  CHECK_GREATER(blockIt->m_allocationCounter, 0, ());
  blockIt->m_allocationCounter--;

  if (blockIt->m_allocationCounter == 0)
  {
    if (m_isInDeallocationSession)
    {
      // Here we set a bit in the deallocation mask to skip the processing of untouched
      // resource collections.
      m_deallocationSessionMask |= (1 << kResourceIndex);
    }
    else
    {
      MemoryBlock memoryBlock = *blockIt;
      it->second.erase(blockIt);
      if (m_sizes[kResourceIndex] > kDesiredSizeInBytes[kResourceIndex])
      {
        CHECK_LESS_OR_EQUAL(memoryBlock.m_blockSize, m_sizes[kResourceIndex], ());
        m_sizes[kResourceIndex] -= memoryBlock.m_blockSize;
        vkFreeMemory(m_device, memoryBlock.m_memory, nullptr);
      }
      else
      {
        memoryBlock.m_freeOffset = 0;
        auto & fm = m_freeBlocks[kResourceIndex];
        fm.push_back(std::move(memoryBlock));
        std::sort(fm.begin(), fm.end());
      }
    }
  }
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
    auto & m = m_memory[i];
    m[i].erase(std::remove_if(m[i].begin(), m[i].end(),
                              [this, &fm, i](MemoryBlock const & b)
    {
      if (b.m_allocationCounter == 0)
      {
        if (m_sizes[i] > kDesiredSizeInBytes[i])
        {
          CHECK_LESS_OR_EQUAL(b.m_blockSize, m_sizes[i], ());
          m_sizes[i] -= b.m_blockSize;
          vkFreeMemory(m_device, b.m_memory, nullptr);
        }
        else
        {
          MemoryBlock block = b;
          block.m_freeOffset = 0;
          fm.push_back(std::move(block));
        }
        return true;
      }
      return false;
    }), m[i].end());
    std::sort(fm.begin(), fm.end());
  }
}
}  // namespace vulkan
}  // namespace dp
