#pragma once

#include "../std/shared_ptr.hpp"
#include "../geometry/rect2d.hpp"

namespace yg
{
  namespace gl
  {
    class VertexBuffer;
    class IndexBuffer;
    class BaseTexture;
    class RenderState;
    class Blitter;

    struct PrepareBackBuffer
    {
      shared_ptr<RenderState> m_renderState;
      shared_ptr<Blitter> m_blitter;

      PrepareBackBuffer(shared_ptr<RenderState> const & renderState,
                        shared_ptr<Blitter> const & blitter);

      void operator()();
    };

    struct DrawGeometry
    {
      m2::RectI m_viewport;

      shared_ptr<BaseTexture> m_texture;
      shared_ptr<VertexBuffer> m_vertices;
      shared_ptr<IndexBuffer> m_indices;
      size_t m_indicesCount;

      DrawGeometry();
      DrawGeometry(m2::RectI const & viewport,
                   shared_ptr<BaseTexture> const & texture,
                   shared_ptr<VertexBuffer> const & vertices,
                   shared_ptr<IndexBuffer> const & indices,
                   size_t indicesCount);

      void operator()();
    };

    struct UpdateActualTarget
    {
      shared_ptr<RenderState> m_renderState;
      shared_ptr<Blitter> m_blitter;

      UpdateActualTarget(shared_ptr<RenderState> const & renderState,
                         shared_ptr<Blitter> const & blitter);
      void operator()();
    };
  }
}
