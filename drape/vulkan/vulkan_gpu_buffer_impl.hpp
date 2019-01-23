#pragma once

#include "drape/data_buffer.hpp"
#include "drape/data_buffer_impl.hpp"
#include "drape/pointers.hpp"
#include "drape/vulkan/vulkan_base_context.hpp"

#include "base/assert.hpp"

#include <vulkan_wrapper.h>
#include <vulkan/vulkan.h>

#include <cstdint>
#include <utility>

namespace dp
{
namespace vulkan
{
class VulkanGPUBuffer : public BufferBase
{
public:
  VulkanGPUBuffer(ref_ptr<VulkanBaseContext> context, void const * data,
                  uint8_t elementSize, uint32_t capacity, uint64_t batcherHash);
  
  void UploadData(ref_ptr<VulkanBaseContext> context, void const * data, uint32_t elementCount);
  
  void * Map(ref_ptr<VulkanBaseContext> context, uint32_t elementOffset, uint32_t elementCount);
  void UpdateData(void * gpuPtr, void const * data,
                  uint32_t elementOffset, uint32_t elementCount);
  void Unmap(ref_ptr<VulkanBaseContext> context);

  VkBuffer GetVulkanBuffer() const { return m_vulkanBuffer; }
  
protected:
  void Resize(ref_ptr<VulkanBaseContext> context, void const * data, uint32_t elementCount);

  VkBuffer m_vulkanBuffer;
  uint64_t m_batcherHash;
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
    m_buffer->UploadData(context, data, elementCount);
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

  VkBuffer GetVulkanBuffer() const { return m_buffer->GetVulkanBuffer(); }

  void Bind() override {}
};
}  // namespace vulkan
}  // namespace dp
