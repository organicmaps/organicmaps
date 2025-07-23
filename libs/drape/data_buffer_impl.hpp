#pragma once

#include "drape/cpu_buffer.hpp"
#include "drape/data_buffer.hpp"
#include "drape/gpu_buffer.hpp"

#include "base/macros.hpp"

#include <cstdint>
#include <utility>

namespace dp
{
// Generic implementation of data buffer.
template <typename TBuffer>
class DataBufferImpl : public DataBufferBase
{
public:
  template <typename... Args>
  DataBufferImpl(Args &&... params) : m_buffer(make_unique_dp<TBuffer>(std::forward<Args>(params)...))
  {}

  uint32_t GetCapacity() const override { return m_buffer->GetCapacity(); }
  uint32_t GetCurrentSize() const override { return m_buffer->GetCurrentSize(); }
  uint32_t GetAvailableSize() const override { return m_buffer->GetAvailableSize(); }
  uint8_t GetElementSize() const override { return m_buffer->GetElementSize(); }
  void Seek(uint32_t elementNumber) override { m_buffer->Seek(elementNumber); }

protected:
  drape_ptr<TBuffer> m_buffer;
};

// CPU implementation of data buffer.
class CpuBufferImpl : public DataBufferImpl<CPUBuffer>
{
public:
  template <typename... Args>
  CpuBufferImpl(Args &&... params) : DataBufferImpl(std::forward<Args>(params)...)
  {}

  void const * Data() const override { return m_buffer->Data(); }

  void UploadData(ref_ptr<GraphicsContext> context, void const * data, uint32_t elementCount) override
  {
    UNUSED_VALUE(context);
    m_buffer->UploadData(data, elementCount);
    uint32_t const newOffset = m_buffer->GetCurrentSize();
    m_buffer->Seek(newOffset);
  }

  void * Map(ref_ptr<GraphicsContext> context, uint32_t elementOffset, uint32_t elementCount) override
  {
    UNUSED_VALUE(context);
    UNUSED_VALUE(elementOffset);
    UNUSED_VALUE(elementCount);
    ASSERT(false, ("Mapping is unavailable for CPU buffer"));
    return nullptr;
  }

  void UpdateData(void * destPtr, void const * srcPtr, uint32_t elementOffset, uint32_t elementCount) override
  {
    UNUSED_VALUE(destPtr);
    UNUSED_VALUE(srcPtr);
    UNUSED_VALUE(elementOffset);
    UNUSED_VALUE(elementCount);
    ASSERT(false, ("Data updating is unavailable for CPU buffer"));
  }

  void Unmap(ref_ptr<GraphicsContext> context) override
  {
    UNUSED_VALUE(context);
    ASSERT(false, ("Unmapping is unavailable for CPU buffer"));
  }

  void Bind() override { ASSERT(false, ("Binding is unavailable for CPU buffer")); }
};

// GPU implementation of data buffer for OpenGL.
class GpuBufferImpl : public DataBufferImpl<GPUBuffer>
{
public:
  template <typename... Args>
  GpuBufferImpl(Args &&... params) : DataBufferImpl(std::forward<Args>(params)...)
  {}

  void const * Data() const override
  {
    ASSERT(false, ("Retrieving of raw data is unavailable for GPU buffer"));
    return nullptr;
  }

  void UploadData(ref_ptr<GraphicsContext> context, void const * data, uint32_t elementCount) override
  {
    UNUSED_VALUE(context);
    m_buffer->UploadData(data, elementCount);
  }

  void * Map(ref_ptr<GraphicsContext> context, uint32_t elementOffset, uint32_t elementCount) override
  {
    UNUSED_VALUE(context);
    return m_buffer->Map(elementOffset, elementCount);
  }

  void UpdateData(void * destPtr, void const * srcPtr, uint32_t elementOffset, uint32_t elementCount) override
  {
    m_buffer->UpdateData(destPtr, srcPtr, elementOffset, elementCount);
  }

  void Unmap(ref_ptr<GraphicsContext> context) override
  {
    UNUSED_VALUE(context);
    m_buffer->Unmap();
  }

  void Bind() override { m_buffer->Bind(); }
};
}  // namespace dp
