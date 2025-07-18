#include "drape/metal/metal_gpu_buffer_impl.hpp"

#include "base/macros.hpp"

#include <cstring>

namespace dp
{
namespace metal
{
MetalGPUBuffer::MetalGPUBuffer(ref_ptr<GraphicsContext> context, void const * data,
                               uint8_t elementSize, uint32_t capacity)
  : BufferBase(elementSize, capacity)
{
  Resize(context, data, capacity);
}

void MetalGPUBuffer::UploadData(void const * data, uint32_t elementCount)
{
  uint32_t const currentSize = GetCurrentSize();
  uint8_t const elementSize = GetElementSize();
  ASSERT(GetCapacity() >= elementCount + currentSize,
         ("Not enough memory to upload ", elementCount, " elements"));
  
  uint32_t const byteOffset = currentSize * elementSize;
  uint32_t const sizeInBytes = elementCount * elementSize;
  uint8_t * gpuPtr = static_cast<uint8_t *>([m_metalBuffer contents]) + byteOffset;
  memcpy(gpuPtr, data, sizeInBytes);
  BufferBase::UploadData(elementCount);
}

void * MetalGPUBuffer::Map(uint32_t elementOffset, uint32_t elementCount)
{
  UNUSED_VALUE(elementCount);
  uint32_t const elementSize = GetElementSize();
  uint32_t const byteOffset = elementOffset * elementSize;
  uint8_t * bufferPointer = static_cast<uint8_t *>([m_metalBuffer contents]) + byteOffset;
  return bufferPointer;
}

void MetalGPUBuffer::UpdateData(void * gpuPtr, void const * data,
                                uint32_t elementOffset, uint32_t elementCount)
{
  uint32_t const elementSize = GetElementSize();
  uint32_t const byteOffset = elementOffset * elementSize;
  uint32_t const byteCount = elementCount * elementSize;

  ASSERT(gpuPtr != nullptr, ());
  memcpy((uint8_t *)gpuPtr + byteOffset, data, byteCount);
}

void MetalGPUBuffer::Resize(ref_ptr<MetalBaseContext> context, void const * data, uint32_t elementCount)
{
  BufferBase::Resize(elementCount);
  
  id<MTLDevice> device = context->GetMetalDevice();
  uint32_t const sizeInBytes = GetCapacity() * GetElementSize();
  if (data != nil)
  {
    m_metalBuffer = [device newBufferWithBytes:data
                                        length:sizeInBytes
                                       options:MTLResourceCPUCacheModeWriteCombined];
  }
  else
  {
    m_metalBuffer = [device newBufferWithLength:sizeInBytes
                                       options:MTLResourceCPUCacheModeWriteCombined];
  }
  
  // If we have already set up data, we have to call SetDataSize.
  if (data != nullptr)
    SetDataSize(elementCount);
}
}  // namespace metal
  
drape_ptr<DataBufferBase> DataBuffer::CreateImplForMetal(ref_ptr<GraphicsContext> context, void const * data,
                                                         uint8_t elementSize, uint32_t capacity)
{
  return make_unique_dp<metal::MetalGpuBufferImpl>(context, data, elementSize, capacity);
}
}  // namespace dp
