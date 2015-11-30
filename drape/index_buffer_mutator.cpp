#include "drape/index_buffer_mutator.hpp"
#include "drape/vertex_array_buffer.hpp"

#include "std/cstring.hpp"    // for memcpy
#include "std/algorithm.hpp" // for max

namespace dp
{

IndexBufferMutator::IndexBufferMutator(uint32_t baseSize)
  : m_activeSize(0)
{
  m_buffer.Resize(baseSize);
}

void IndexBufferMutator::AppendIndexes(void const * indexes, uint32_t count)
{
  uint32_t dstActiveSize = m_activeSize + count;
  if (dstActiveSize  > m_buffer.Size())
    m_buffer.Resize(max(m_buffer.Size() * 2, dstActiveSize));

  memcpy(m_buffer.GetRaw(m_activeSize), indexes, count * IndexStorage::SizeOfIndex());
  m_activeSize = dstActiveSize;
}

void const * IndexBufferMutator::GetIndexes() const
{
  return m_buffer.GetRawConst();
}

uint32_t IndexBufferMutator::GetIndexCount() const
{
  return m_activeSize;
}

} // namespace dp
