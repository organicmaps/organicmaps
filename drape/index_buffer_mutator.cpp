#include "drape/index_buffer_mutator.hpp"
#include "drape/vertex_array_buffer.hpp"

#include "std/cstring.hpp"    // for memcpy
#include "std/algorithm.hpp" // for max

namespace dp
{

IndexBufferMutator::IndexBufferMutator(uint16_t baseSize)
  : m_activeSize(0)
{
  m_buffer.resize(baseSize);
}

void IndexBufferMutator::AppendIndexes(uint16_t const * indexes, uint16_t count)
{
  size_t dstActiveSize = m_activeSize + count;
  if (dstActiveSize  > m_buffer.size())
    m_buffer.resize(max(m_buffer.size() * 2, dstActiveSize));

  memcpy(&m_buffer[m_activeSize], indexes, count * sizeof(uint16_t));
  m_activeSize = dstActiveSize;
}

uint16_t const * IndexBufferMutator::GetIndexes() const
{
  return &m_buffer[0];
}

uint16_t IndexBufferMutator::GetIndexCount() const
{
  return m_activeSize;
}

} // namespace dp
