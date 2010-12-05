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

    Storage::Storage(size_t vbSize, size_t ibSize) :
      m_vertices(new VertexBuffer(vbSize)),
      m_indices(new IndexBuffer(ibSize))
    {}
  }
}
