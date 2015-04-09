#pragma once

#include "std/shared_ptr.hpp"

namespace graphics
{
  namespace gl
  {
    class BufferObject;

    class Storage
    {
    public:
      shared_ptr<BufferObject> m_vertices;
      shared_ptr<BufferObject> m_indices;

      Storage();
      Storage(size_t vbSize, size_t ibSize);

      bool isValid() const;
    };
  }
}

