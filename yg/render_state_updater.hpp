#pragma once

#include "../std/shared_ptr.hpp"
#include "../base/timer.hpp"

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

      int m_indicesCount;
      bool m_doPeriodicalUpdate;
      double m_updateInterval;
      my::Timer m_updateTimer;

    public:

      struct Params : base_t::Params
      {
        bool m_doPeriodicalUpdate;
        double m_updateInterval;
        shared_ptr<RenderState> m_renderState;
        Params();
      };

      RenderStateUpdater(Params const & params);

      shared_ptr<RenderState> const & renderState() const;

      void drawGeometry(shared_ptr<BaseTexture> const & texture,
                        shared_ptr<VertexBuffer> const & vertices,
                        shared_ptr<IndexBuffer> const & indices,
                        size_t indicesCount);

      void beginFrame();
      void endFrame();
      void setClipRect(m2::RectI const & rect);
      virtual void updateActualTarget();
    };
  }
}
