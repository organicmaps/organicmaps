#include "../base/SRC_FIRST.hpp"

#include "render_state_updater.hpp"
#include "render_state.hpp"
#include "framebuffer.hpp"

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
      LOG(LINFO, ("UpdateInterval: ", m_updateInterval));
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

      /// blitting will be performed through
      /// non-multisampled framebuffer for the sake of speed
      if (isMultiSampled())
        frameBuffer()->makeCurrent();

      OGLCHECK(glFinish());

      {
        threads::MutexGuard guard(*m_renderState->m_mutex.get());

        m_renderState->m_actualTarget->attachToFrameBuffer();

        OGLCHECK(glDisable(GL_SCISSOR_TEST));

        OGLCHECK(glClearColor(192 / 255.0, 192 / 255.0, 192 / 255.0, 1.0));
        OGLCHECK(glClear(GL_COLOR_BUFFER_BIT));

        shared_ptr<BaseTexture> backBuffer = m_renderState->m_backBufferLayers.front();

        immDrawTexturedRect(
            m2::RectF(0, 0, backBuffer->width(), backBuffer->height()),
            m2::RectF(0, 0, 1, 1),
            backBuffer
            );

/*        for (int i = 1; i < m_renderState->m_backBufferLayers.size(); ++i)
        {
          shared_ptr<BaseTexture> layer = m_renderState->m_backBufferLayers[i];
          immDrawTexturedRect(
              m2::RectF(0, 0, layer->width(), layer->height()),
              m2::RectF(0, 0, 1, 1),
              layer);
        }*/

        if (clipRectEnabled())
          OGLCHECK(glEnable(GL_SCISSOR_TEST));

        m_renderState->m_actualScreen = m_renderState->m_currentScreen;

        m_renderState->m_backBufferLayers.front()->attachToFrameBuffer();

        OGLCHECK(glFinish());
      }

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

    void RenderStateUpdater::endFrame()
    {
      if (m_renderState)
        updateActualTarget();
      base_t::endFrame();
    }
  }
}
