#pragma once

#include "../std/shared_ptr.hpp"

#include "blitter.hpp"

namespace yg
{
  namespace gl
  {
    class VertexBuffer;
    class IndexBuffer;
    class BaseTexture;

    class GeometryRenderer : public Blitter
    {
    public:
      void drawGeometry(shared_ptr<BaseTexture> const & texture,
                        shared_ptr<VertexBuffer> const & vertices,
                        shared_ptr<IndexBuffer> const & indices,
                        size_t indicesCount);
    };
  }
}

