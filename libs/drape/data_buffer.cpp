#include "drape/data_buffer.hpp"
#include "drape/data_buffer_impl.hpp"
#include "drape/drape_global.hpp"

namespace dp
{
DataBuffer::DataBuffer(uint8_t elementSize, uint32_t capacity)
  : m_impl(make_unique_dp<CpuBufferImpl>(elementSize, capacity))
{}

ref_ptr<DataBufferBase> DataBuffer::GetBuffer() const
{
  ASSERT(m_impl != nullptr, ());
  return make_ref(m_impl);
}

void DataBuffer::MoveToGPU(ref_ptr<GraphicsContext> context, GPUBuffer::Target target, uint64_t batcherHash)
{
  // Cache values from m_impl before replacing it.
  uint32_t const currentSize = m_impl->GetCurrentSize();
  void const * data = currentSize != 0 ? m_impl->Data() : nullptr;
  uint8_t const elementSize = m_impl->GetElementSize();
  uint32_t const availableSize = m_impl->GetAvailableSize();

  auto const apiVersion = context->GetApiVersion();
  if (apiVersion == dp::ApiVersion::OpenGLES3)
  {
    if (currentSize != 0)
      m_impl = make_unique_dp<GpuBufferImpl>(target, data, elementSize, currentSize, batcherHash);
    else
      m_impl = make_unique_dp<GpuBufferImpl>(target, nullptr, elementSize, availableSize, batcherHash);
  }
  else if (apiVersion == dp::ApiVersion::Metal)
  {
#if defined(OMIM_METAL_AVAILABLE)
    if (currentSize != 0)
      m_impl = CreateImplForMetal(context, data, elementSize, currentSize);
    else
      m_impl = CreateImplForMetal(context, nullptr, elementSize, availableSize);
#endif
  }
  else if (apiVersion == dp::ApiVersion::Vulkan)
  {
    if (currentSize != 0)
      m_impl = CreateImplForVulkan(context, data, elementSize, currentSize, batcherHash);
    else
      m_impl = CreateImplForVulkan(context, nullptr, elementSize, availableSize, batcherHash);
  }
  else
  {
    CHECK(false, ("Unsupported API version."));
  }
}

DataBufferMapper::DataBufferMapper(ref_ptr<GraphicsContext> context, ref_ptr<DataBuffer> buffer, uint32_t elementOffset,
                                   uint32_t elementCount)
  : m_context(context)
  , m_buffer(buffer)
{
  m_buffer->GetBuffer()->Bind();
  m_ptr = m_buffer->GetBuffer()->Map(m_context, elementOffset, elementCount);
}

DataBufferMapper::~DataBufferMapper()
{
  m_buffer->GetBuffer()->Unmap(m_context);
}

void DataBufferMapper::UpdateData(void const * data, uint32_t elementOffset, uint32_t elementCount)
{
  m_buffer->GetBuffer()->UpdateData(m_ptr, data, elementOffset, elementCount);
}
}  // namespace dp
