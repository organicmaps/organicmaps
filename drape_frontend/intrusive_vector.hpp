#pragma once

#include "base/assert.hpp"

#include "std/cstdint.hpp"

namespace df
{

enum FillDirection
{
  Forward,
  Backward
};

template <typename T>
class IntrusiveVector
{
public:
  IntrusiveVector(void * memoryBuffer, uint32_t byteCount)
    : m_memory(reinterpret_cast<T *>(memoryBuffer))
    , m_direction(Forward)
  {
    ASSERT(byteCount % sizeof(T) == 0, ());
    m_capacity = byteCount / sizeof(T);
    m_size = 0;
  }

  void SetFillDirection(FillDirection direction)
  {
    ASSERT(m_size == 0, ());
    m_direction = direction;
  }

  void PushBack(T const & value)
  {
    ASSERT(m_size < m_capacity, ());
    if (m_direction == Forward)
      m_memory[m_size++] = value;
    else
    {
      m_memory[m_capacity - m_size - 1] = value;
      m_size++;
    }
  }

private:
  T * m_memory;
  FillDirection m_direction;
  uint32_t m_capacity;
  uint32_t m_size;
};

}
