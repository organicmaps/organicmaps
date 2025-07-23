#pragma once

#include "drape/data_buffer.hpp"
#include "drape/data_buffer_impl.hpp"
#include "drape/pointers.hpp"
#include "drape/vulkan/vulkan_base_context.hpp"
#include "drape/vulkan/vulkan_object_manager.hpp"

#include "base/assert.hpp"

#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

namespace dp
{
namespace vulkan
{
class VulkanStagingBuffer;

class VulkanGPUBuffer : public BufferBase
{
public:
  VulkanGPUBuffer(ref_ptr<VulkanBaseContext> context, void const * data, uint8_t elementSize, uint32_t capacity,
                  uint64_t batcherHash);
  ~VulkanGPUBuffer() override;

  void * Map(ref_ptr<VulkanBaseContext> context, uint32_t elementOffset, uint32_t elementCount);
  void UpdateData(void * gpuPtr, void const * data, uint32_t elementOffset, uint32_t elementCount);
  void Unmap(ref_ptr<VulkanBaseContext> context);

  void Advance(uint32_t elementCount) { BufferBase::UploadData(elementCount); }

  VkBuffer GetVulkanBuffer() const { return m_geometryBuffer.m_buffer; }

protected:
  void Resize(ref_ptr<VulkanBaseContext> context, void const * data, uint32_t elementCount);

  ref_ptr<VulkanObjectManager> m_objectManager;
  VulkanObject m_geometryBuffer;
  uint64_t m_batcherHash;

  ref_ptr<VulkanStagingBuffer> m_stagingBufferRef;
  drape_ptr<VulkanStagingBuffer> m_ownStagingBuffer;
  uint32_t m_reservationId = 0;
  uint32_t m_mappingByteOffset = 0;
  uint32_t m_mappingByteOffsetMin = std::numeric_limits<uint32_t>::max();
  uint32_t m_mappingByteOffsetMax = std::numeric_limits<uint32_t>::min();
  std::vector<VkBufferCopy> m_regionsToCopy;
};

class VulkanGpuBufferImpl : public DataBufferImpl<VulkanGPUBuffer>
{
public:
  template <typename... Args>
  VulkanGpuBufferImpl(Args &&... params) : DataBufferImpl(std::forward<Args>(params)...)
  {}

  void const * Data() const override
  {
    ASSERT(false, ("Retrieving of raw data is unavailable for GPU buffer"));
    return nullptr;
  }

  void UploadData(ref_ptr<GraphicsContext> context, void const * data, uint32_t elementCount) override
  {
    // In Vulkan we must call upload only from FR.
    ref_ptr<VulkanBaseContext> vulkanContext = context;
    CHECK(vulkanContext->GetCurrentMemoryCommandBuffer() != nullptr, ());

    uint32_t const currentSize = m_buffer->GetCurrentSize();
    ASSERT(m_buffer->GetCapacity() >= elementCount + currentSize,
           ("Not enough memory to upload ", elementCount, " elements"));
    auto ptr = m_buffer->Map(context, currentSize, elementCount);
    CHECK(ptr != nullptr, ());
    m_buffer->UpdateData(ptr, data, 0, elementCount);
    m_buffer->Unmap(context);
    m_buffer->Advance(elementCount);
  }

  void UpdateData(void * destPtr, void const * srcPtr, uint32_t elementOffset, uint32_t elementCount) override
  {
    m_buffer->UpdateData(destPtr, srcPtr, elementOffset, elementCount);
  }

  void * Map(ref_ptr<GraphicsContext> context, uint32_t elementOffset, uint32_t elementCount) override
  {
    return m_buffer->Map(context, elementOffset, elementCount);
  }

  void Unmap(ref_ptr<GraphicsContext> context) override { m_buffer->Unmap(context); }

  void Bind() override {}

  VkBuffer GetVulkanBuffer() const { return m_buffer->GetVulkanBuffer(); }
};
}  // namespace vulkan
}  // namespace dp
