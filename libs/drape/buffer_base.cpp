#include "drape/buffer_base.hpp"
#include "base/assert.hpp"

namespace dp
{

BufferBase::BufferBase(uint8_t elementSize, uint32_t capacity)
  : m_elementSize(elementSize)
  , m_capacity(capacity)
  , m_size(0)
{}

uint32_t BufferBase::GetCapacity() const
{
  return m_capacity;
}

uint32_t BufferBase::GetCurrentSize() const
{
  return m_size;
}

uint32_t BufferBase::GetAvailableSize() const
{
  return m_capacity - m_size;
}

void BufferBase::Resize(uint32_t elementCount)
{
  m_capacity = elementCount;
  m_size = 0;
}

uint8_t BufferBase::GetElementSize() const
{
  return m_elementSize;
}

void BufferBase::Seek(uint32_t elementNumber)
{
  ASSERT(elementNumber <= m_capacity, ());
  m_size = elementNumber;
}

void BufferBase::UploadData(uint32_t elementCount)
{
  ASSERT(m_size + elementCount <= m_capacity, ());
  m_size += elementCount;
}

void BufferBase::SetDataSize(uint32_t elementCount)
{
  ASSERT(elementCount <= m_capacity, ());
  m_size = elementCount;
}

}  // namespace dp
