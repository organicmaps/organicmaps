#include "../base/SRC_FIRST.hpp"
#include "render_state_updater.hpp"
#include "render_state.hpp"
#include "framebuffer.hpp"

namespace yg
{
  namespace gl
  {
    void RenderStateUpdater::setRenderState(shared_ptr<RenderState> const & renderState)
    {
      m_renderState = renderState;
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
      if ((m_renderState) && (m_indicesCount > 20000))
      {
        updateActualTarget();
        m_indicesCount %= 20000;
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
//        setRenderTarget(m_renderState->m_actualTarget, false);

        OGLCHECK(glDisable(GL_SCISSOR_TEST));

        OGLCHECK(glClearColor(192 / 255.0, 192 / 255.0, 192 / 255.0, 1.0));
        OGLCHECK(glClear(GL_COLOR_BUFFER_BIT));

        size_t w = m_renderState->m_backBuffer->width();
        size_t h = m_renderState->m_backBuffer->height();

        immDrawTexturedRect(
            m2::RectF(0, 0, w, h),
            m2::RectF(0, 0, 1, 1),
            m_renderState->m_backBuffer
            );

        if (clipRectEnabled())
          OGLCHECK(glEnable(GL_SCISSOR_TEST));

        OGLCHECK(glFinish());

        m_renderState->m_actualScreen = m_renderState->m_currentScreen;

//        setRenderTarget(m_renderState->m_backBuffer);

//        frameBuffer()->attachTexture(m_renderState->m_backBuffer);

        m_renderState->m_backBuffer->attachToFrameBuffer();
      }

      if (isMultiSampled())
        multiSampledFrameBuffer()->makeCurrent();

      m_renderState->invalidate();
    }

    void RenderStateUpdater::beginFrame()
    {
      base_t::beginFrame();
      m_indicesCount = 0;
    }

    void RenderStateUpdater::endFrame()
    {
      if (m_indicesCount && m_renderState)
        updateActualTarget();
      base_t::endFrame();
    }
  }
}
