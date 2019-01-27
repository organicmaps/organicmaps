#include "drape/vulkan/vulkan_object_manager.hpp"
#include "drape/vulkan/vulkan_utils.hpp"

#include <algorithm>

namespace dp
{
namespace vulkan
{
uint32_t constexpr kDefaultStagingBufferSizeInBytes = 10 * 1024 * 1024;

VulkanObjectManager::VulkanObjectManager(VkDevice device, VkPhysicalDeviceLimits const & deviceLimits,
                                         VkPhysicalDeviceMemoryProperties const & memoryProperties,
                                         uint32_t queueFamilyIndex)
  : m_device(device)
  , m_queueFamilyIndex(queueFamilyIndex)
  , m_memoryManager(device, deviceLimits, memoryProperties)
{
  m_queueToDestroy.reserve(50);

  m_defaultStagingBuffer = CreateBuffer(VulkanMemoryManager::ResourceType::Staging,
                                        kDefaultStagingBufferSizeInBytes, 0 /* batcherHash */);
  VkMemoryRequirements memReqs = {};
  vkGetBufferMemoryRequirements(m_device, m_defaultStagingBuffer.m_buffer, &memReqs);
  m_defaultStagingBufferAlignment = m_memoryManager.GetSizeAlignment(memReqs);

  CHECK_VK_CALL(vkBindBufferMemory(device, m_defaultStagingBuffer.m_buffer,
                                   m_defaultStagingBuffer.m_allocation->m_memory,
                                   m_defaultStagingBuffer.m_allocation->m_alignedOffset));

  CHECK_VK_CALL(vkMapMemory(device, m_defaultStagingBuffer.m_allocation->m_memory,
                            m_defaultStagingBuffer.m_allocation->m_alignedOffset,
                            m_defaultStagingBuffer.m_allocation->m_alignedSize, 0,
                            reinterpret_cast<void **>(&m_defaultStagingBufferPtr)));
}

VulkanObjectManager::~VulkanObjectManager()
{
  vkUnmapMemory(m_device, m_defaultStagingBuffer.m_allocation->m_memory);
  DestroyObject(m_defaultStagingBuffer);
  CollectObjects();
}

VulkanObject VulkanObjectManager::CreateBuffer(VulkanMemoryManager::ResourceType resourceType,
                                               uint32_t sizeInBytes, uint64_t batcherHash)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  VulkanObject result;
  VkBufferCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  info.pNext = nullptr;
  info.flags = 0;
  info.size = sizeInBytes;
  if (resourceType == VulkanMemoryManager::ResourceType::Geometry)
    info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  else if (resourceType == VulkanMemoryManager::ResourceType::Uniform)
    info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  else if (resourceType == VulkanMemoryManager::ResourceType::Staging)
    info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  else
    CHECK(false, ("Unsupported resource type."));

  info.usage = VK_SHARING_MODE_EXCLUSIVE;
  info.queueFamilyIndexCount = 1;
  info.pQueueFamilyIndices = &m_queueFamilyIndex;
  CHECK_VK_CALL(vkCreateBuffer(m_device, &info, nullptr, &result.m_buffer));

  VkMemoryRequirements memReqs = {};
  vkGetBufferMemoryRequirements(m_device, result.m_buffer, &memReqs);

  result.m_allocation = m_memoryManager.Allocate(resourceType, memReqs, batcherHash);
  return result;
}

VulkanObject VulkanObjectManager::CreateImage(VkImageUsageFlagBits usageFlagBits, VkFormat format,
                                              VkImageAspectFlags aspectFlags, uint32_t width, uint32_t height)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  VulkanObject result;
  VkImageCreateInfo imageCreateInfo = {};
  imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageCreateInfo.pNext = nullptr;
  imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
  imageCreateInfo.format = format;
  imageCreateInfo.mipLevels = 1;
  imageCreateInfo.arrayLayers = 1;
  imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageCreateInfo.extent = { width, height, 1 };
  imageCreateInfo.usage = usageFlagBits | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  CHECK_VK_CALL(vkCreateImage(m_device, &imageCreateInfo, nullptr, &result.m_image));

  VkImageViewCreateInfo viewCreateInfo = {};
  viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewCreateInfo.pNext = nullptr;
  viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewCreateInfo.format = format;
  viewCreateInfo.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
                               VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
  viewCreateInfo.subresourceRange.aspectMask = aspectFlags;
  viewCreateInfo.subresourceRange.baseMipLevel = 0;
  viewCreateInfo.subresourceRange.levelCount = 1;
  viewCreateInfo.subresourceRange.baseArrayLayer = 0;
  viewCreateInfo.subresourceRange.layerCount = 1;
  viewCreateInfo.image = result.m_image;
  CHECK_VK_CALL(vkCreateImageView(m_device, &viewCreateInfo, nullptr, &result.m_imageView));

  VkMemoryRequirements memReqs = {};
  vkGetImageMemoryRequirements(m_device, result.m_image, &memReqs);

  result.m_allocation = m_memoryManager.Allocate(VulkanMemoryManager::ResourceType::Image,
                                                 memReqs, 0 /* blockHash */);
  return result;
}

void VulkanObjectManager::DestroyObject(VulkanObject object)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_queueToDestroy.push_back(std::move(object));
}

VulkanObjectManager::StagingPointer VulkanObjectManager::GetDefaultStagingBuffer(uint32_t sizeInBytes)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  auto const alignedSize = m_memoryManager.GetAligned(sizeInBytes, m_defaultStagingBufferAlignment);
  if (m_defaultStagingBufferOffset + alignedSize > kDefaultStagingBufferSizeInBytes)
    return {};

  auto const alignedOffset = m_defaultStagingBufferOffset;
  uint8_t * ptr = m_defaultStagingBufferPtr + alignedOffset;

  // Update offset and align it.
  m_defaultStagingBufferOffset += alignedSize;
  auto const alignment = m_memoryManager.GetOffsetAlignment(VulkanMemoryManager::ResourceType::Staging);
  m_defaultStagingBufferOffset = std::min(m_memoryManager.GetAligned(m_defaultStagingBufferOffset, alignment),
                                          m_defaultStagingBuffer.m_allocation->m_alignedSize);

  StagingPointer stagingDataPtr;
  stagingDataPtr.m_stagingData.m_stagingBuffer = m_defaultStagingBuffer.m_buffer;
  stagingDataPtr.m_stagingData.m_offset = alignedOffset;
  stagingDataPtr.m_pointer = ptr;
  return stagingDataPtr;
}

VulkanObjectManager::StagingData VulkanObjectManager::CopyWithDefaultStagingBuffer(uint32_t sizeInBytes,
                                                                                   void * data)
{
  auto s = GetDefaultStagingBuffer(sizeInBytes);
  memcpy(s.m_pointer, data, sizeInBytes);
  return s.m_stagingData;
}

VulkanObjectManager::StagingData VulkanObjectManager::CopyWithTemporaryStagingBuffer(uint32_t sizeInBytes,
                                                                                     void * data)
{
  auto stagingObj = CreateBuffer(VulkanMemoryManager::ResourceType::Staging,
                                 sizeInBytes, 0 /* batcherHash */);
  void * gpuPtr = nullptr;
  CHECK_VK_CALL(vkMapMemory(m_device, stagingObj.m_allocation->m_memory,
                            stagingObj.m_allocation->m_alignedOffset,
                            stagingObj.m_allocation->m_alignedSize, 0, &gpuPtr));
  memcpy(gpuPtr, data, sizeInBytes);
  if (!stagingObj.m_allocation->m_isCoherent)
  {
    VkMappedMemoryRange mappedRange = {};
    mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory = stagingObj.m_allocation->m_memory;
    mappedRange.offset = stagingObj.m_allocation->m_alignedOffset;
    mappedRange.size = stagingObj.m_allocation->m_alignedSize;
    CHECK_VK_CALL(vkFlushMappedMemoryRanges(m_device, 1, &mappedRange));
  }
  vkUnmapMemory(m_device, stagingObj.m_allocation->m_memory);
  CHECK_VK_CALL(vkBindBufferMemory(m_device, stagingObj.m_buffer,
                                   stagingObj.m_allocation->m_memory,
                                   stagingObj.m_allocation->m_alignedOffset));

  StagingData stagingData;
  stagingData.m_stagingBuffer = stagingObj.m_buffer;
  stagingData.m_offset = 0;

  // The object will be destroyed on the next CollectObjects().
  DestroyObject(stagingObj);
  return stagingData;
}

void VulkanObjectManager::FlushDefaultStagingBuffer()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_defaultStagingBuffer.m_allocation->m_isCoherent || m_defaultStagingBufferOffset == 0)
    return;

  VkMappedMemoryRange mappedRange = {};
  mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
  mappedRange.memory = m_defaultStagingBuffer.m_allocation->m_memory;
  mappedRange.offset = m_defaultStagingBuffer.m_allocation->m_alignedOffset;
  mappedRange.size = mappedRange.offset + m_defaultStagingBufferOffset;
  CHECK_VK_CALL(vkFlushMappedMemoryRanges(m_device, 1, &mappedRange));
}

void VulkanObjectManager::ResetDefaultStagingBuffer()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_defaultStagingBufferOffset = 0;
}

void VulkanObjectManager::CollectObjects()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_queueToDestroy.empty())
    return;

  m_memoryManager.BeginDeallocationSession();
  for (size_t i = 0; i < m_queueToDestroy.size(); ++i)
  {
    if (m_queueToDestroy[i].m_buffer != 0)
      vkDestroyBuffer(m_device, m_queueToDestroy[i].m_buffer, nullptr);
    if (m_queueToDestroy[i].m_imageView != 0)
      vkDestroyImageView(m_device, m_queueToDestroy[i].m_imageView, nullptr);
    if (m_queueToDestroy[i].m_image != 0)
      vkDestroyImage(m_device, m_queueToDestroy[i].m_image, nullptr);

    if (m_queueToDestroy[i].m_allocation)
      m_memoryManager.Deallocate(m_queueToDestroy[i].m_allocation);
  }
  m_memoryManager.EndDeallocationSession();
  m_queueToDestroy.clear();
}
}  // namespace vulkan
}  // namespace dp
