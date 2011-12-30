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

      struct UploadData : Command
      {
        vector<shared_ptr<ResourceStyle> > m_styles;
        shared_ptr<BaseTexture> m_texture;
        void perform();
      };

      struct DrawGeometry : Command
      {
        shared_ptr<BaseTexture> m_texture;
        shared_ptr<VertexBuffer> m_vertices;
        shared_ptr<IndexBuffer> m_indices;
        size_t m_indicesCount;
        void perform();
      };

      GeometryRenderer(base_t::Params const & params);

      void uploadData(vector<shared_ptr<ResourceStyle> > const & v,
                      shared_ptr<BaseTexture> const & texture);

      void applyStates(bool isAntiAliased);

      void drawGeometry(shared_ptr<BaseTexture> const & texture,
                        shared_ptr<VertexBuffer> const & vertices,
                        shared_ptr<IndexBuffer> const & indices,
                        size_t indicesCount);
    };
  }
}

