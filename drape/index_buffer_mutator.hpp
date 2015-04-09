#pragma once

#include "drape/pointers.hpp"

namespace dp
{

class VertexArrayBuffer;

class IndexBufferMutator
{
public:
  IndexBufferMutator(uint16_t baseSize);

  void AppendIndexes(uint16_t const * indexes, uint16_t count);

private:
  friend class VertexArrayBuffer;
  uint16_t const * GetIndexes() const;
  uint16_t GetIndexCount() const;

private:
  vector<uint16_t> m_buffer;
  uint16_t m_activeSize;
};

} // namespace dp
