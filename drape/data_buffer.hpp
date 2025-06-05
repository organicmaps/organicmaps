#pragma once

#include "drape/gpu_buffer.hpp"
#include "drape/graphics_context.hpp"
#include "drape/pointers.hpp"

namespace dp
{
class DataBufferBase
{
public:
  virtual ~DataBufferBase() = default;
  virtual uint32_t GetCapacity() const = 0;
  virtual uint32_t GetCurrentSize() const = 0;
  virtual uint32_t GetAvailableSize() const = 0;
  virtual uint8_t GetElementSize() const = 0;
  virtual void Seek(uint32_t elementNumber) = 0;
  virtual void const * Data() const = 0;

  virtual void UploadData(ref_ptr<GraphicsContext> context, void const * data, uint32_t elementCount) = 0;

  virtual void * Map(ref_ptr<GraphicsContext> context, uint32_t elementOffset, uint32_t elementCount) = 0;
  virtual void UpdateData(void * destPtr, void const * srcPtr, uint32_t elementOffset, uint32_t elementCount) = 0;
  virtual void Unmap(ref_ptr<GraphicsContext> context) = 0;

  virtual void Bind() = 0;
};

class DataBuffer
{
public:
  DataBuffer(uint8_t elementSize, uint32_t capacity);

  ref_ptr<DataBufferBase> GetBuffer() const;
  void MoveToGPU(ref_ptr<GraphicsContext> context, GPUBuffer::Target target, uint64_t batcherHash);

private:
  // Definition of this method is in a .mm-file.
  drape_ptr<DataBufferBase> CreateImplForMetal(ref_ptr<GraphicsContext> context, void const * data, uint8_t elementSize,
                                               uint32_t capacity);

  // Definition of this method is in a separate .cpp-file.
  drape_ptr<DataBufferBase> CreateImplForVulkan(ref_ptr<GraphicsContext> context, void const * data,
                                                uint8_t elementSize, uint32_t capacity, uint64_t batcherHash);

  drape_ptr<DataBufferBase> m_impl;
};

class DataBufferMapper
{
public:
  DataBufferMapper(ref_ptr<GraphicsContext> context, ref_ptr<DataBuffer> buffer, uint32_t elementOffset,
                   uint32_t elementCount);
  ~DataBufferMapper();

  void UpdateData(void const * data, uint32_t elementOffset, uint32_t elementCount);

private:
  ref_ptr<GraphicsContext> m_context;
  ref_ptr<DataBuffer> m_buffer;
  void * m_ptr;
};
}  // namespace dp
