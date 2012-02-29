#include "../base/SRC_FIRST.hpp"

#include "render_state_updater.hpp"
#include "render_state.hpp"
#include "renderbuffer.hpp"
#include "framebuffer.hpp"
#include "base_texture.hpp"
#include "utils.hpp"

#include "internal/opengl.hpp"

#include "../base/logging.hpp"
#include "../std/bind.hpp"

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
      m_auxFrameBuffer(params.m_auxFrameBuffer),
      m_doPeriodicalUpdate(params.m_doPeriodicalUpdate),
      m_updateInterval(params.m_updateInterval)
    {
      if ((m_doPeriodicalUpdate) && (!m_auxFrameBuffer))
      {
        m_auxFrameBuffer.reset();
        m_auxFrameBuffer.reset(new FrameBuffer());
      }
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

    void RenderStateUpdater::UpdateBackBuffer::perform()
    {
      if (isDebugging())
        LOG(LINFO, ("performing UpdateBackBuffer command"));

      if (m_doSynchronize)
        m_renderState->m_mutex->Lock();

      OGLCHECK(glFinish());

      OGLCHECK(glDisable(GL_SCISSOR_TEST));

      m_auxFrameBuffer->setRenderTarget(m_renderState->m_backBuffer);
      m_auxFrameBuffer->makeCurrent();

      OGLCHECK(glClearColor(s_bgColor.r / 255.0,
                            s_bgColor.g / 255.0,
                            s_bgColor.b / 255.0,
                            s_bgColor.a / 255.0));

      OGLCHECK(glClear(GL_COLOR_BUFFER_BIT));

      shared_ptr<IMMDrawTexturedRect> immDrawTexturedRect(
            new IMMDrawTexturedRect(m2::RectF(0, 0, m_renderState->m_actualTarget->width(), m_renderState->m_actualTarget->height()),
                                    m2::RectF(0, 0, 1, 1),
                                    m_renderState->m_actualTarget,
                                    m_resourceManager));

      immDrawTexturedRect->setIsDebugging(isDebugging());
      immDrawTexturedRect->perform();

      m_frameBuffer->makeCurrent();

      if (m_isClipRectEnabled)
        OGLCHECK(glEnable(GL_SCISSOR_TEST));

      OGLCHECK(glScissor(m_clipRect.minX(), m_clipRect.minY(), m_clipRect.maxX(), m_clipRect.maxY()));

      OGLCHECK(glFinish());

      if (m_doSynchronize)
        m_renderState->m_mutex->Unlock();
    }

    void RenderStateUpdater::Invalidate::perform()
    {
      m_renderState->invalidate();
    }

    void RenderStateUpdater::updateActualTarget()
    {
      /// Carefully synchronizing the access to the m_renderState to minimize wait time.
      finish();

      completeCommands();

      /// to re-set the states
      base_t::endFrame();
      base_t::beginFrame();

      m_renderState->m_mutex->Lock();

      swap(m_renderState->m_actualTarget, m_renderState->m_backBuffer);
      m_renderState->m_actualScreen = m_renderState->m_currentScreen;
      if (!m_renderState->m_isEmptyModelCurrent)
        m_renderState->m_isEmptyModelActual = m_renderState->m_isEmptyModelCurrent;

      m_renderState->m_mutex->Unlock();

      base_t::endFrame();

      setRenderTarget(m_renderState->m_backBuffer);

      shared_ptr<UpdateBackBuffer> command1(new UpdateBackBuffer());

      command1->m_renderState = m_renderState;
      command1->m_resourceManager = resourceManager();
      command1->m_isClipRectEnabled = clipRectEnabled();
      command1->m_clipRect = clipRect();
      command1->m_doSynchronize = renderQueue();
      command1->m_auxFrameBuffer = m_auxFrameBuffer;
      command1->m_frameBuffer = frameBuffer();

      base_t::beginFrame();

      processCommand(command1);

      m_renderState->invalidate();

      shared_ptr<Invalidate> command2(new Invalidate());
      command2->m_renderState = m_renderState;

      processCommand(command2);

      base_t::endFrame();
      completeCommands();

      setRenderTarget(m_renderState->m_backBuffer);
      base_t::beginFrame();
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
