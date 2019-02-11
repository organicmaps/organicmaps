#pragma once

#include "drape/data_buffer.hpp"
#include "drape/data_buffer_impl.hpp"
#include "drape/pointers.hpp"
#include "drape/vulkan/vulkan_base_context.hpp"
#include "drape/vulkan/vulkan_object_manager.hpp"

#include "base/assert.hpp"

#include <vulkan_wrapper.h>
#include <vulkan/vulkan.h>

#include <cstdint>
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
  VulkanGPUBuffer(ref_ptr<VulkanBaseContext> context, void const * data,
                  uint8_t elementSize, uint32_t capacity, uint64_t batcherHash);

  void * Map(ref_ptr<VulkanBaseContext> context, uint32_t elementOffset, uint32_t elementCount);
  void UpdateData(void * gpuPtr, void const * data,
                  uint32_t elementOffset, uint32_t elementCount);
  void Unmap(ref_ptr<VulkanBaseContext> context);

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
  std::vector<VkBufferCopy> m_regionsToCopy;
  std::vector<VkBufferMemoryBarrier> m_barriers;
};
  
class VulkanGpuBufferImpl : public DataBufferImpl<VulkanGPUBuffer>
{
public:
  template <typename... Args>
  VulkanGpuBufferImpl(Args &&... params)
    : DataBufferImpl(std::forward<Args>(params)...)
  {}
  
  void const * Data() const override
  {
    ASSERT(false, ("Retrieving of raw data is unavailable for GPU buffer"));
    return nullptr;
  }
  
  void UploadData(ref_ptr<GraphicsContext> context, void const * data,
                  uint32_t elementCount) override
  {
    // For Vulkan we can't update buffers by means of UploadData, because UploadData
    // can be called from BR, where command buffers are not available.
    CHECK(false, ("UploadData is unsupported for Vulkan buffers (use Map-Copy-Unmap)."));
  }
  
  void UpdateData(void * destPtr, void const * srcPtr, uint32_t elementOffset,
                  uint32_t elementCount) override
  {
    m_buffer->UpdateData(destPtr, srcPtr, elementOffset, elementCount);
  }
  
  void * Map(ref_ptr<GraphicsContext> context, uint32_t elementOffset,
             uint32_t elementCount) override
  {
    return m_buffer->Map(context, elementOffset, elementCount);
  }

  void Unmap(ref_ptr<GraphicsContext> context) override
  {
    m_buffer->Unmap(context);
  }

  void Bind() override {}

  VkBuffer GetVulkanBuffer() const { return m_buffer->GetVulkanBuffer(); }
};
}  // namespace vulkan
}  // namespace dp
