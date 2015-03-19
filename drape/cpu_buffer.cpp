#include "drape/cpu_buffer.hpp"

#include "base/math.hpp"
#include "base/shared_buffer_manager.hpp"
#include "base/assert.hpp"

#include "std/cstring.hpp"

namespace dp
{

CPUBuffer::CPUBuffer(uint8_t elementSize, uint16_t capacity)
  : TBase(elementSize, capacity)
{
  uint32_t memorySize = my::NextPowOf2(GetCapacity() * GetElementSize());
  m_memory = SharedBufferManager::instance().reserveSharedBuffer(memorySize);
  m_memoryCursor = NonConstData();
}

CPUBuffer::~CPUBuffer()
{
  m_memoryCursor = NULL;
  SharedBufferManager::instance().freeSharedBuffer(m_memory->size(), m_memory);
}

void CPUBuffer::UploadData(void const * data, uint16_t elementCount, bool const advanceCursor)
{
  uint32_t byteCountToCopy = GetElementSize() * elementCount;
#ifdef DEBUG
  // Memory check
  ASSERT(GetCursor() + byteCountToCopy <= Data() + m_memory->size(), ());
#endif

  memcpy(GetCursor(), data, byteCountToCopy);
  TBase::UploadData(elementCount);

  if (advanceCursor)
  {
    uint32_t offsetFromBegin = GetElementSize() * GetCurrentSize();
    m_memoryCursor = NonConstData() + offsetFromBegin;
  }
}

void CPUBuffer::Seek(uint16_t elementNumber)
{
  uint32_t offsetFromBegin = GetElementSize() * elementNumber;
  ASSERT(Data() + offsetFromBegin <= Data() + m_memory->size(), ());
  TBase::Seek(elementNumber);
  m_memoryCursor = NonConstData() + offsetFromBegin;
}

uint16_t CPUBuffer::GetCurrentElementNumber() const
{
  uint16_t pointerDiff = GetCursor() - Data();
  ASSERT(pointerDiff % GetElementSize() == 0, ());
  return pointerDiff / GetElementSize();
}

unsigned char const * CPUBuffer::Data() const
{
  return &((*m_memory)[0]);
}

unsigned char * CPUBuffer::NonConstData()
{
  return &((*m_memory)[0]);
}

unsigned char * CPUBuffer::GetCursor() const
{
  return m_memoryCursor;
}

} // namespace dp
