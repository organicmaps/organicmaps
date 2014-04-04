#include "index_buffer_mutator.hpp"
#include "vertex_array_buffer.hpp"

IndexBufferMutator::IndexBufferMutator(uint16_t baseSize)
  : m_activeSize(0)
{
  m_buffer.resize(baseSize);
}

void IndexBufferMutator::AppendIndexes(uint16_t const * indexes, uint16_t count)
{
  if (m_activeSize + count > m_buffer.size())
    m_buffer.resize(m_buffer.size() * 2);

  memcpy(&m_buffer[m_activeSize], indexes, count * sizeof(uint16_t));
  m_activeSize += count;
}

void IndexBufferMutator::Submit(RefPointer<VertexArrayBuffer> vao)
{
  vao->UpdateIndexBuffer(&m_buffer[0], m_activeSize);
}
