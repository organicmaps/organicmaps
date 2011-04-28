#include "../base/SRC_FIRST.hpp"
#include "storage.hpp"
#include "vertexbuffer.hpp"
#include "indexbuffer.hpp"

namespace yg
{
  namespace gl
  {
    Storage::Storage()
    {}

    Storage::Storage(size_t vbSize, size_t ibSize, bool useVA) :
      m_vertices(new VertexBuffer(vbSize, useVA)),
      m_indices(new IndexBuffer(ibSize, useVA))
    {}
  }
}
