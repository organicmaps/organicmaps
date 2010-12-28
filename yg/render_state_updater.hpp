#pragma once

#include "../std/shared_ptr.hpp"
#include "../geometry/screenbase.hpp"

#include "geometry_renderer.hpp"

namespace yg
{
  namespace gl
  {
    class RenderState;

    class RenderStateUpdater : public GeometryRenderer
    {
    private:

      typedef GeometryRenderer base_t;

      shared_ptr<RenderState> m_renderState;

      void updateActualTarget();

      int m_indicesCount;

    public:

      RenderStateUpdater(base_t::Params const & params);

      void setRenderState(shared_ptr<RenderState> const & renderState);
      shared_ptr<RenderState> const & renderState() const;

      void drawGeometry(shared_ptr<BaseTexture> const & texture,
                        shared_ptr<VertexBuffer> const & vertices,
                        shared_ptr<IndexBuffer> const & indices,
                        size_t indicesCount);

      void beginFrame();
      void endFrame();
    };
  }
}
