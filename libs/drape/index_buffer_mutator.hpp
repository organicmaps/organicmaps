#pragma once

#include "drape/index_storage.hpp"
#include "drape/pointers.hpp"

#include <cstdint>

namespace dp
{
class VertexArrayBuffer;

class IndexBufferMutator
{
public:
  explicit IndexBufferMutator(uint32_t baseSize);

  void AppendIndexes(void const * indexes, uint32_t count);

  uint32_t GetCapacity() const;

private:
  friend class VertexArrayBuffer;
  void const * GetIndexes() const;
  uint32_t GetIndexCount() const;

private:
  IndexStorage m_buffer;
  uint32_t m_activeSize = 0;
};
}  // namespace dp
