#pragma once

#include "blitter.hpp"

#include "../base/threaded_list.hpp"

#include "../std/shared_ptr.hpp"
#include "../std/function.hpp"

namespace yg
{
  struct ResourceStyle;

  namespace gl
  {
    class VertexBuffer;
    class IndexBuffer;
    class BaseTexture;

    class GeometryRenderer : public Blitter
    {
    public:

      typedef Blitter base_t;

      struct DrawGeometry : Command
      {
        shared_ptr<BaseTexture> m_texture;
        shared_ptr<VertexBuffer> m_vertices;
        shared_ptr<IndexBuffer> m_indices;
        size_t m_indicesCount;
        void perform();
      };

      GeometryRenderer(base_t::Params const & params);

      void applyStates(bool isAntiAliased);

      void drawGeometry(shared_ptr<BaseTexture> const & texture,
                        shared_ptr<VertexBuffer> const & vertices,
                        shared_ptr<IndexBuffer> const & indices,
                        size_t indicesCount);
    };
  }
}

