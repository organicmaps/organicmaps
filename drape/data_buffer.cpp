#include "drape/data_buffer.hpp"
#include "drape/data_buffer_impl.hpp"

namespace dp
{

DataBuffer::DataBuffer(uint8_t elementSize, uint16_t capacity)
  : m_impl(make_unique_dp<CpuBufferImpl>(elementSize, capacity))
{
}

DataBuffer::~DataBuffer()
{
}

ref_ptr<DataBufferBase> DataBuffer::GetBuffer() const
{
  ASSERT(m_impl != nullptr, ());
  return make_ref<DataBufferBase>(m_impl);
}

void DataBuffer::MoveToGPU(GPUBuffer::Target target)
{
  // if currentSize is 0 buffer hasn't been filled on preparation stage, let it be filled further
  uint16_t const currentSize = m_impl->GetCurrentSize();
  if (currentSize != 0)
  {
    drape_ptr<DataBufferBase> newImpl = make_unique_dp<GpuBufferImpl>(target, m_impl->Data(),
                                                                      m_impl->GetElementSize(),
                                                                      currentSize);
    m_impl.reset();
    m_impl = move(newImpl);
  }
  else
  {
    drape_ptr<DataBufferBase> newImpl = make_unique_dp<GpuBufferImpl>(target, nullptr,
                                                                      m_impl->GetElementSize(),
                                                                      m_impl->GetAvailableSize());
    m_impl.reset();
    m_impl = move(newImpl);
  }
}


DataBufferMapper::DataBufferMapper(ref_ptr<DataBuffer> buffer)
  : m_buffer(buffer)
{
  m_buffer->GetBuffer()->Bind();
  m_ptr = m_buffer->GetBuffer()->Map();
}

DataBufferMapper::~DataBufferMapper()
{
  m_buffer->GetBuffer()->Unmap();
}

void DataBufferMapper::UpdateData(void const * data, uint16_t elementOffset, uint16_t elementCount)
{
  m_buffer->GetBuffer()->UpdateData(m_ptr, data, elementOffset, elementCount);
}

}
