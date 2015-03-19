#include "drape/data_buffer.hpp"
#include "drape/data_buffer_impl.hpp"

namespace dp
{

DataBuffer::DataBuffer(uint8_t elementSize, uint16_t capacity)
{
  m_impl.Reset(new CpuBufferImpl(elementSize, capacity));
}

DataBuffer::~DataBuffer()
{
  m_impl.Destroy();
}

dp::RefPointer<DataBufferBase> DataBuffer::GetBuffer() const
{
  return m_impl.GetRefPointer();
}

void DataBuffer::MoveToGPU(GPUBuffer::Target target)
{
  dp::MasterPointer<DataBufferBase> newImpl;

  // if currentSize is 0 buffer hasn't been filled on preparation stage, let it be filled further
  uint16_t const currentSize = m_impl->GetCurrentSize();
  if (currentSize != 0)
    newImpl.Reset(new GpuBufferImpl(target, m_impl->Data(), m_impl->GetElementSize(), currentSize));
  else
    newImpl.Reset(new GpuBufferImpl(target, nullptr, m_impl->GetElementSize(), m_impl->GetAvailableSize()));

  m_impl.Destroy();
  m_impl = newImpl;
}


DataBufferMapper::DataBufferMapper(RefPointer<DataBuffer> buffer)
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
