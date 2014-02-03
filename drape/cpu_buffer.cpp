#include "cpu_buffer.hpp"

#include "../base/math.hpp"
#include "../base/shared_buffer_manager.hpp"
#include "../base/assert.hpp"

CPUBuffer::CPUBuffer(uint8_t elementSize, uint16_t capacity)
  : base_t(elementSize, capacity)
{
  m_memorySize = my::NextPowOf2(GetCapacity() * GetElementSize());
  SharedBufferManager::instance().reserveSharedBuffer(m_memorySize);
  m_memoryCursor = GetBufferBegin();
}

CPUBuffer::~CPUBuffer()
{
  m_memoryCursor = NULL;
  SharedBufferManager::instance().freeSharedBuffer(m_memorySize, m_memory);
}

void CPUBuffer::UploadData(const void * data, uint16_t elementCount)
{
  uint32_t byteCountToCopy = GetElementSize() * elementCount;
#ifdef DEBUG
  // Memory check
  ASSERT(GetCursor() + byteCountToCopy <= GetBufferBegin() + m_memorySize, ());
#endif

  memcpy(GetCursor(), data, byteCountToCopy);
  base_t::UploadData(elementCount);
}

void CPUBuffer::Seek(uint16_t elementNumber)
{
  uint32_t offsetFromBegin = GetElementSize() * elementNumber;
  ASSERT(GetBufferBegin() + offsetFromBegin <= GetBufferBegin() + m_memorySize, ());
  base_t::Seek(elementNumber);
  m_memoryCursor = GetBufferBegin() + offsetFromBegin;
}

uint16_t CPUBuffer::GetCurrentElementNumber() const
{
  uint16_t pointerDiff = GetCursor() - GetBufferBegin();
  ASSERT(pointerDiff % GetElementSize() == 0, ());
  return pointerDiff / GetElementSize();
}

unsigned char * CPUBuffer::GetBufferBegin() const
{
  return &((*m_memory)[0]);
}

unsigned char * CPUBuffer::GetCursor() const
{
  return m_memoryCursor;
}
