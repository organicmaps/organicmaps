#pragma once

#include "data_buffer.hpp"
#include "cpu_buffer.hpp"
#include "gpu_buffer.hpp"

#include "std/utility.hpp"

namespace dp
{

/// generic implementation of data buffer
template <typename TBuffer>
class DataBufferImpl : public DataBufferBase
{
public:
  template<typename... Args> DataBufferImpl(Args&&... params)
  {
    m_buffer.Reset(new TBuffer(forward<Args>(params)...));
  }

  ~DataBufferImpl() override
  {
    m_buffer.Destroy();
  }

  uint16_t GetCapacity() const override
  {
    return m_buffer->GetCapacity();
  }

  uint16_t GetCurrentSize() const override
  {
    return m_buffer->GetCurrentSize();
  }

  uint16_t GetAvailableSize() const override
  {
    return m_buffer->GetAvailableSize();
  }

  uint8_t GetElementSize() const override
  {
    return m_buffer->GetElementSize();
  }

  void Seek(uint16_t elementNumber) override
  {
    m_buffer->Seek(elementNumber);
  }

protected:
  dp::MasterPointer<TBuffer> m_buffer;
};

/// CPU implementation of data buffer
class CpuBufferImpl : public DataBufferImpl<CPUBuffer>
{
public:
  template<typename... Args> CpuBufferImpl(Args&&... params)
    : DataBufferImpl(forward<Args>(params)...)
  {}

  void const * Data() const override
  {
    return m_buffer->Data();
  }

  void UploadData(void const * data, uint16_t elementCount) override
  {
    m_buffer->UploadData(data, elementCount);
    uint16_t const newOffset = m_buffer->GetCurrentSize();
    m_buffer->Seek(newOffset);
  }

  void UpdateData(void * destPtr, void const * srcPtr, uint16_t elementOffset, uint16_t elementCount) override
  {
    ASSERT(false, ("Data updating is unavailable for CPU buffer"));
  }

  void Bind() override
  {
    ASSERT(false, ("Binding is unavailable for CPU buffer"));
  }

  void * Map() override
  {
    ASSERT(false, ("Mapping is unavailable for CPU buffer"));
    return nullptr;
  }

  void Unmap() override
  {
    ASSERT(false, ("Unmapping is unavailable for CPU buffer"));
  }
};

/// GPU implementation of data buffer
class GpuBufferImpl : public DataBufferImpl<GPUBuffer>
{
public:
  template<typename... Args> GpuBufferImpl(Args&&... params)
    : DataBufferImpl(forward<Args>(params)...)
  {}

  void const * Data() const override
  {
    ASSERT(false, ("Retrieving of raw data is unavailable for GPU buffer"));
    return nullptr;
  }

  void UploadData(void const * data, uint16_t elementCount) override
  {
    m_buffer->UploadData(data, elementCount);
  }

  void UpdateData(void * destPtr, void const * srcPtr, uint16_t elementOffset, uint16_t elementCount) override
  {
    m_buffer->UpdateData(destPtr, srcPtr, elementOffset, elementCount);
  }

  void Bind() override
  {
    m_buffer->Bind();
  }

  void * Map() override
  {
    return m_buffer->Map();
  }

  void Unmap() override
  {
    return m_buffer->Unmap();
  }
};

} // namespace dp
