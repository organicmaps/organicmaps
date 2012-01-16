#pragma once

#include "../std/shared_ptr.hpp"
#include "../geometry/screenbase.hpp"
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
      shared_ptr<FrameBuffer> m_auxFrameBuffer;

      int m_indicesCount;
      bool m_doPeriodicalUpdate;
      double m_updateInterval;
      my::Timer m_updateTimer;

      struct UpdateBackBuffer : Command
      {
        shared_ptr<RenderState> m_renderState;
        shared_ptr<ResourceManager> m_resourceManager;
        shared_ptr<FrameBuffer> m_auxFrameBuffer;
        shared_ptr<FrameBuffer> m_frameBuffer;
        bool m_isClipRectEnabled;
        m2::RectI m_clipRect;
        bool m_doSynchronize;

        void perform();
      };

      struct Invalidate : Command
      {
        shared_ptr<RenderState> m_renderState;
        void perform();
      };

    public:

      struct Params : base_t::Params
      {
        bool m_doPeriodicalUpdate;
        double m_updateInterval;
        shared_ptr<RenderState> m_renderState;
        shared_ptr<FrameBuffer> m_auxFrameBuffer;
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
      void updateActualTarget();
    };
  }
}
