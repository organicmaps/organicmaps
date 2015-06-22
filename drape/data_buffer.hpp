#pragma once

#include "drape/pointers.hpp"
#include "drape/gpu_buffer.hpp"

namespace dp
{

class DataBufferBase
{
public:
  virtual ~DataBufferBase() {}

  virtual uint32_t GetCapacity() const = 0;
  virtual uint32_t GetCurrentSize() const = 0;
  virtual uint32_t GetAvailableSize() const = 0;
  virtual uint8_t GetElementSize() const = 0;
  virtual void Seek(uint32_t elementNumber) = 0;
  virtual void const * Data() const = 0;

  virtual void UploadData(void const * data, uint32_t elementCount) = 0;
  virtual void UpdateData(void * destPtr, void const * srcPtr, uint32_t elementOffset, uint32_t elementCount) = 0;

  virtual void Bind() = 0;
  virtual void * Map() = 0;
  virtual void Unmap() = 0;
};


class DataBuffer
{
public:
  DataBuffer(uint8_t elementSize, uint32_t capacity);

  ref_ptr<DataBufferBase> GetBuffer() const;
  void MoveToGPU(GPUBuffer::Target target);

private:
  drape_ptr<DataBufferBase> m_impl;
};


class DataBufferMapper
{
public:
  DataBufferMapper(ref_ptr<DataBuffer> buffer);
  ~DataBufferMapper();

  void UpdateData(void const * data, uint32_t elementOffset, uint32_t elementCount);

private:
  ref_ptr<DataBuffer> m_buffer;
  void * m_ptr;
};

} // namespace dp

