#include "drape/vulkan/vulkan_gpu_buffer_impl.hpp"
#include "drape/vulkan/vulkan_staging_buffer.hpp"
#include "drape/vulkan/vulkan_utils.hpp"

#include "base/macros.hpp"

#include <algorithm>
#include <cstring>

namespace dp
{
namespace vulkan
{
VulkanGPUBuffer::VulkanGPUBuffer(ref_ptr<VulkanBaseContext> context, void const * data, uint8_t elementSize,
                                 uint32_t capacity, uint64_t batcherHash)
  : BufferBase(elementSize, capacity)
  , m_batcherHash(batcherHash)
{
  m_regionsToCopy.reserve(5);
  Resize(context, data, capacity);
}

VulkanGPUBuffer::~VulkanGPUBuffer()
{
  CHECK(m_objectManager != nullptr, ());
  m_objectManager->DestroyObject(m_geometryBuffer);
}

void * VulkanGPUBuffer::Map(ref_ptr<VulkanBaseContext> context, uint32_t elementOffset, uint32_t elementCount)
{
  CHECK(m_objectManager != nullptr, ());

  uint32_t const elementSize = GetElementSize();
  uint32_t const mappingSizeInBytes = elementCount * elementSize;
  m_mappingByteOffset = elementOffset * elementSize;
  m_mappingByteOffsetMin = std::numeric_limits<uint32_t>::max();
  m_mappingByteOffsetMax = std::numeric_limits<uint32_t>::min();

  VkCommandBuffer commandBuffer = context->GetCurrentMemoryCommandBuffer();
  CHECK(commandBuffer != nullptr, ());

  // Copy to default or temporary staging buffer.
  m_stagingBufferRef = context->GetDefaultStagingBuffer();
  if (!m_stagingBufferRef->HasEnoughSpace(mappingSizeInBytes))
  {
    m_ownStagingBuffer = make_unique_dp<VulkanStagingBuffer>(m_objectManager, mappingSizeInBytes);
    ASSERT(m_ownStagingBuffer->HasEnoughSpace(mappingSizeInBytes), ());
    m_stagingBufferRef = make_ref(m_ownStagingBuffer);
  }

  VulkanStagingBuffer::StagingData data;
  m_reservationId = m_stagingBufferRef->ReserveWithId(mappingSizeInBytes, data);
  return data.m_pointer;
}

void VulkanGPUBuffer::UpdateData(void * gpuPtr, void const * data, uint32_t elementOffset, uint32_t elementCount)
{
  CHECK(gpuPtr != nullptr, ());
  CHECK(m_stagingBufferRef != nullptr, ());
  uint32_t const elementSize = GetElementSize();
  uint32_t const byteOffset = elementOffset * elementSize;
  uint32_t const byteCount = elementCount * elementSize;
  memcpy(static_cast<uint8_t *>(gpuPtr) + byteOffset, data, byteCount);

  uint32_t const baseSrcOffset = m_stagingBufferRef->GetReservationById(m_reservationId).m_offset;
  VkBufferCopy copyRegion = {};
  copyRegion.dstOffset = m_mappingByteOffset + byteOffset;
  copyRegion.srcOffset = baseSrcOffset + byteOffset;
  copyRegion.size = byteCount;

  m_mappingByteOffsetMin = std::min(m_mappingByteOffsetMin, static_cast<uint32_t>(copyRegion.dstOffset));
  m_mappingByteOffsetMax =
      std::max(m_mappingByteOffsetMax, static_cast<uint32_t>(copyRegion.dstOffset + copyRegion.size));

  m_regionsToCopy.push_back(std::move(copyRegion));
}

void VulkanGPUBuffer::Unmap(ref_ptr<VulkanBaseContext> context)
{
  VkCommandBuffer commandBuffer = context->GetCurrentMemoryCommandBuffer();
  CHECK(commandBuffer != nullptr, ());

  VkBuffer stagingBuffer = m_stagingBufferRef->GetReservationById(m_reservationId).m_stagingBuffer;
  if (m_ownStagingBuffer != nullptr)
  {
    m_ownStagingBuffer->Flush();
    m_ownStagingBuffer.reset();
  }
  m_stagingBufferRef = nullptr;

  CHECK_LESS(m_mappingByteOffsetMin, m_mappingByteOffsetMax, ());

  // Set up a barrier to prevent data collisions (write-after-write, write-after-read).
  VkBufferMemoryBarrier barrier = {};
  barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  barrier.pNext = nullptr;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT;
  barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.buffer = m_geometryBuffer.m_buffer;
  barrier.offset = m_mappingByteOffsetMin;
  barrier.size = m_mappingByteOffsetMax - m_mappingByteOffsetMin;
  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);

  // Schedule command to copy from the staging buffer to the geometry buffer.
  vkCmdCopyBuffer(commandBuffer, stagingBuffer, m_geometryBuffer.m_buffer,
                  static_cast<uint32_t>(m_regionsToCopy.size()), m_regionsToCopy.data());

  // Set up barriers to prevent data collisions (read-after-write).
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr,
                       1, &barrier, 0, nullptr);

  m_mappingByteOffset = 0;
  m_regionsToCopy.clear();
}

void VulkanGPUBuffer::Resize(ref_ptr<VulkanBaseContext> context, void const * data, uint32_t elementCount)
{
  BufferBase::Resize(elementCount);

  m_objectManager = context->GetObjectManager();
  uint32_t const sizeInBytes = GetCapacity() * GetElementSize();

  m_geometryBuffer =
      m_objectManager->CreateBuffer(VulkanMemoryManager::ResourceType::Geometry, sizeInBytes, m_batcherHash);
  m_objectManager->Fill(m_geometryBuffer, data, sizeInBytes);

  // If we have already set up data, we have to call SetDataSize.
  if (data != nullptr)
    SetDataSize(elementCount);
}
}  // namespace vulkan

drape_ptr<DataBufferBase> DataBuffer::CreateImplForVulkan(ref_ptr<GraphicsContext> context, void const * data,
                                                          uint8_t elementSize, uint32_t capacity, uint64_t batcherHash)
{
  return make_unique_dp<vulkan::VulkanGpuBufferImpl>(context, data, elementSize, capacity, batcherHash);
}
}  // namespace dp
