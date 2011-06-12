#include "render_state_updater.hpp"
#include "render_state.hpp"
#include "framebuffer.hpp"
#include "internal/opengl.hpp"
#include "base_texture.hpp"
#include "info_layer.hpp"

#include "../base/logging.hpp"

namespace yg
{
  namespace gl
  {
    RenderStateUpdater::Params::Params()
      : m_doPeriodicalUpdate(false), m_updateInterval(0.0)
    {}

    RenderStateUpdater::RenderStateUpdater(Params const & params)
      : base_t(params),
      m_renderState(params.m_renderState),
      m_doPeriodicalUpdate(params.m_doPeriodicalUpdate),
      m_updateInterval(params.m_updateInterval)
    {
    }

    shared_ptr<RenderState> const & RenderStateUpdater::renderState() const
    {
      return m_renderState;
    }

    void RenderStateUpdater::drawGeometry(shared_ptr<BaseTexture> const & texture,
                                          shared_ptr<VertexBuffer> const & vertices,
                                          shared_ptr<IndexBuffer> const & indices,
                                          size_t indicesCount)
    {
      base_t::drawGeometry(texture, vertices, indices, indicesCount);
      m_indicesCount += indicesCount;
      if (m_doPeriodicalUpdate
       && m_renderState
       && (m_indicesCount > 20000)
       && (m_updateTimer.ElapsedSeconds() > m_updateInterval))
      {
        updateActualTarget();
        m_indicesCount %= 20000;
        m_updateTimer.Reset();
      }
    }

    void RenderStateUpdater::updateActualTarget()
    {
      /// Carefully synchronizing the access to the m_renderState to minimize wait time.
      OGLCHECK(glFinish());
      updateFrameBuffer();

      {
        threads::MutexGuard guard(*m_renderState->m_mutex.get());
        swap(m_renderState->m_actualTarget, m_renderState->m_backBufferLayers.front());
//        swap(m_renderState->m_actualInfoLayer, m_renderState->m_currentInfoLayer);
        m_renderState->m_actualScreen = m_renderState->m_currentScreen;
      }

      /// copying info layer
//      *m_renderState->m_currentInfoLayer.get() = *m_renderState->m_actualInfoLayer.get();

      /// blitting will be performed through
      /// non-multisampled framebuffer for the sake of speed

      frameBuffer()->setRenderTarget(m_renderState->m_backBufferLayers.front());
      frameBuffer()->makeCurrent();

      OGLCHECK(glFinish());

      OGLCHECK(glDisable(GL_SCISSOR_TEST));

      OGLCHECK(glClearColor(192 / 255.0, 192 / 255.0, 192 / 255.0, 1.0));
      OGLCHECK(glClear(GL_COLOR_BUFFER_BIT));

      shared_ptr<BaseTexture> actualTarget = m_renderState->m_actualTarget;

      immDrawTexturedRect(
          m2::RectF(0, 0, actualTarget->width(), actualTarget->height()),
          m2::RectF(0, 0, 1, 1),
          actualTarget
          );

      if (clipRectEnabled())
        OGLCHECK(glEnable(GL_SCISSOR_TEST));

      OGLCHECK(glFinish());

      if (isMultiSampled())
        multiSampledFrameBuffer()->makeCurrent();

      m_renderState->invalidate();
    }

    void RenderStateUpdater::beginFrame()
    {
      base_t::beginFrame();
      m_indicesCount = 0;
      m_updateTimer.Reset();
    }

    void RenderStateUpdater::setClipRect(m2::RectI const & rect)
    {
      if ((m_renderState) && (m_indicesCount))
      {
        updateActualTarget();
        m_indicesCount = 0;
        m_updateTimer.Reset();
      }

      base_t::setClipRect(rect);
    }

    void RenderStateUpdater::endFrame()
    {
      if (m_renderState)
        updateActualTarget();
      m_indicesCount = 0;
      m_updateTimer.Reset();
      base_t::endFrame();
    }
  }
}
