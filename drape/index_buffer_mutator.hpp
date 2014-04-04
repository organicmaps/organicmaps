#pragma once

#include "pointers.hpp"

class VertexArrayBuffer;

class IndexBufferMutator
{
public:
  IndexBufferMutator(uint16_t baseSize);

  void AppendIndexes(uint16_t const * indexes, uint16_t count);
  void Submit(RefPointer<VertexArrayBuffer> vao);

private:
  vector<uint16_t> m_buffer;
  uint16_t m_activeSize;
};
