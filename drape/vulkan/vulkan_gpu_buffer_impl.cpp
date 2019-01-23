#include "drape/vulkan/vulkan_gpu_buffer_impl.hpp"

#include "base/macros.hpp"

#include <cstring>

namespace dp
{
namespace vulkan
{
VulkanGPUBuffer::VulkanGPUBuffer(ref_ptr<VulkanBaseContext> context, void const * data,
                                 uint8_t elementSize, uint32_t capacity, uint64_t batcherHash)
  : BufferBase(elementSize, capacity)
  , m_batcherHash(batcherHash)
{
  Resize(context, data, capacity);
}

void VulkanGPUBuffer::UploadData(ref_ptr<VulkanBaseContext> context, void const * data,
                                 uint32_t elementCount)
{
  //TODO: Upload must be called only, there is no active command buffer.
  uint32_t const currentSize = GetCurrentSize();
  uint8_t const elementSize = GetElementSize();
  ASSERT(GetCapacity() >= elementCount + currentSize,
         ("Not enough memory to upload ", elementCount, " elements"));
  
  uint32_t const byteOffset = currentSize * elementSize;
  uint32_t const sizeInBytes = elementCount * elementSize;
  CHECK(false, ());
//  uint8_t * gpuPtr = static_cast<uint8_t *>([m_metalBuffer contents]) + byteOffset;
//  memcpy(gpuPtr, data, sizeInBytes);
  BufferBase::UploadData(elementCount);
}

void * VulkanGPUBuffer::Map(ref_ptr<VulkanBaseContext> context, uint32_t elementOffset,
                            uint32_t elementCount)
{
  //TODO: Stage + map.
  UNUSED_VALUE(elementCount);
  uint32_t const elementSize = GetElementSize();
  uint32_t const byteOffset = elementOffset * elementSize;
  CHECK(false, ());
  uint8_t * bufferPointer = nullptr;//static_cast<uint8_t *>([m_metalBuffer contents]) + byteOffset;
  return bufferPointer;
}

void VulkanGPUBuffer::UpdateData(void * gpuPtr, void const * data,
                                 uint32_t elementOffset, uint32_t elementCount)
{
  uint32_t const elementSize = GetElementSize();
  uint32_t const byteOffset = elementOffset * elementSize;
  uint32_t const byteCount = elementCount * elementSize;

  ASSERT(gpuPtr != nullptr, ());
  memcpy((uint8_t *)gpuPtr + byteOffset, data, byteCount);
}

void VulkanGPUBuffer::Unmap(ref_ptr<VulkanBaseContext> context)
{
  //TODO: Unmap + barrier.
}

void VulkanGPUBuffer::Resize(ref_ptr<VulkanBaseContext> context, void const * data,
                             uint32_t elementCount)
{
  BufferBase::Resize(elementCount);

  CHECK(false, ());
//  id<MTLDevice> device = context->GetMetalDevice();
//  uint32_t const sizeInBytes = GetCapacity() * GetElementSize();
//  if (data != nil)
//  {
//    m_metalBuffer = [device newBufferWithBytes:data
//                                        length:sizeInBytes
//                                       options:MTLResourceCPUCacheModeWriteCombined];
//  }
//  else
//  {
//    m_metalBuffer = [device newBufferWithLength:sizeInBytes
//                                       options:MTLResourceCPUCacheModeWriteCombined];
//  }
  
  // If we have already set up data, we have to call SetDataSize.
  if (data != nullptr)
    SetDataSize(elementCount);
}
}  // namespace vulkan
  
drape_ptr<DataBufferBase> DataBuffer::CreateImplForVulkan(ref_ptr<GraphicsContext> context, void const * data,
                                                          uint8_t elementSize, uint32_t capacity,
                                                          uint64_t batcherHash)
{
  return make_unique_dp<vulkan::VulkanGpuBufferImpl>(context, data, elementSize, capacity, batcherHash);
}
}  // namespace dp
