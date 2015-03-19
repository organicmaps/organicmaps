#include "drape/buffer_base.hpp"
#include "base/assert.hpp"

namespace dp
{

BufferBase::BufferBase(uint8_t elementSize, uint16_t capacity)
  : m_elementSize(elementSize)
  , m_capacity(capacity)
  , m_size(0)
{
}

uint16_t BufferBase::GetCapacity() const
{
  return m_capacity;
}

uint16_t BufferBase::GetCurrentSize() const
{
  return m_size;
}

uint16_t BufferBase::GetAvailableSize() const
{
  return m_capacity - m_size;
}

void BufferBase::Resize(uint16_t elementCount)
{
  m_capacity = elementCount;
  m_size = 0;
}

uint8_t BufferBase::GetElementSize() const
{
  return m_elementSize;
}

void BufferBase::Seek(uint16_t elementNumber)
{
  ASSERT(elementNumber <= m_capacity, ());
  m_size = elementNumber;
}

void BufferBase::UploadData(uint16_t elementCount)
{
  ASSERT(m_size + elementCount <= m_capacity, ());
  m_size += elementCount;
}

void BufferBase::SetDataSize(uint16_t elementCount)
{
  ASSERT(elementCount <= m_capacity, ());
  m_size = elementCount;
}

} // namespace dp
