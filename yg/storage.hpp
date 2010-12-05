#pragma once

#include "../std/shared_ptr.hpp"

namespace yg
{
  namespace gl
  {
    class VertexBuffer;
    class IndexBuffer;

    class Storage
    {
    public:
      shared_ptr<VertexBuffer> m_vertices;
      shared_ptr<IndexBuffer> m_indices;

      Storage();
      Storage(size_t vbSize, size_t ibSize);
    };
  }
}

