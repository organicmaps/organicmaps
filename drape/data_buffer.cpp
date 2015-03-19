#include "drape/data_buffer.hpp"

namespace dp
{

DataBuffer::DataBuffer(GPUBuffer::Target target, uint8_t elementSize, uint16_t capacity) :
  m_target(target)
{
  m_cpuBuffer.Reset(new CPUBuffer(elementSize, capacity));
}

DataBuffer::~DataBuffer()
{
  if (!m_cpuBuffer.IsNull())
    m_cpuBuffer.Destroy();

  if (!m_gpuBuffer.IsNull())
    m_gpuBuffer.Destroy();
}

void DataBuffer::UploadData(void const * data, uint16_t elementCount)
{
  if (!m_cpuBuffer.IsNull())
  {
    ASSERT(m_gpuBuffer.IsNull(), ("GPU buffer must not exist until CPU buffer is alive"));
    m_cpuBuffer->UploadData(data, elementCount);
    uint16_t const newOffset = m_cpuBuffer->GetCurrentSize();
    m_cpuBuffer->Seek(newOffset);
  }
  else
  {
    ASSERT(!m_gpuBuffer.IsNull(), ("GPU buffer must be alive here"));
    m_gpuBuffer->UploadData(data, elementCount);
  }
}

BufferBase const * DataBuffer::GetActiveBuffer() const
{
  if (!m_cpuBuffer.IsNull())
  {
    ASSERT(m_gpuBuffer.IsNull(), ());
    return m_cpuBuffer.GetRaw();
  }

  ASSERT(!m_gpuBuffer.IsNull(), ());
  return m_gpuBuffer.GetRaw();
}

void DataBuffer::Seek(uint16_t elementNumber)
{
  if (!m_cpuBuffer.IsNull())
    m_cpuBuffer->Seek(elementNumber);
  else
    m_gpuBuffer->Seek(elementNumber);
}

uint16_t DataBuffer::GetCapacity() const
{
  return GetActiveBuffer()->GetCapacity();
}

uint16_t DataBuffer::GetCurrentSize() const
{
  return GetActiveBuffer()->GetCurrentSize();
}

uint16_t DataBuffer::GetAvailableSize() const
{
  return GetActiveBuffer()->GetAvailableSize();
}

void DataBuffer::Bind()
{
  ASSERT(!m_gpuBuffer.IsNull(), ("GPU buffer must be alive here"));
  m_gpuBuffer->Bind();
}

dp::RefPointer<GPUBuffer> DataBuffer::GetGpuBuffer() const
{
  ASSERT(!m_gpuBuffer.IsNull(), ("GPU buffer must be alive here"));
  return m_gpuBuffer.GetRefPointer();
}

dp::RefPointer<CPUBuffer> DataBuffer::GetСpuBuffer() const
{
  ASSERT(!m_cpuBuffer.IsNull(), ("CPU buffer must be alive here"));
  return m_cpuBuffer.GetRefPointer();
}

void DataBuffer::MoveToGPU()
{
  ASSERT(m_gpuBuffer.IsNull() && !m_cpuBuffer.IsNull(), ("Duplicate buffer's moving from CPU to GPU"));

  uint8_t const elementSize = m_cpuBuffer->GetElementSize();

  // if currentSize is 0 buffer hasn't been filled on preparation stage, let it be filled further
  uint16_t const currentSize = m_cpuBuffer->GetCurrentSize();
  if (currentSize != 0)
    m_gpuBuffer.Reset(new GPUBuffer(m_target, m_cpuBuffer->Data(), elementSize, currentSize));
  else
    m_gpuBuffer.Reset(new GPUBuffer(m_target, nullptr, elementSize, m_cpuBuffer->GetAvailableSize()));

  m_cpuBuffer.Destroy();
}

}
