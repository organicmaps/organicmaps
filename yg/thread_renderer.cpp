#include "../base/SRC_FIRST.hpp"
#include "thread_renderer.hpp"
#include "framebuffer.hpp"
#include "blitter.hpp"
#include "rendercontext.hpp"
#include "texture.hpp"
#include "renderbuffer.hpp"
#include "render_state.hpp"
#include "vertexbuffer.hpp"
#include "indexbuffer.hpp"
#include "vertex.hpp"

#include "../base/logging.hpp"
#include "../geometry/screenbase.hpp"
#include "../std/algorithm.hpp"

namespace yg
{
  namespace gl
  {
    ThreadRenderer::ThreadRenderer()
    {}

    void ThreadRenderer::init(shared_ptr<RenderContext> const & /*renderContext*/,
                              shared_ptr<RenderState> const & /*renderState*/)
    {
//      m_renderContext = renderContext;
//      m_renderState = renderState;
    }

    void ThreadRenderer::setupStates()
    {
/*      LOG(LINFO, ("initializing separate thread rendering"));
      m_renderContext->makeCurrent();

      m_blitter = make_shared_ptr(new Blitter());
      m_blitter->setFrameBuffer(make_shared_ptr(new FrameBuffer()));

      OGLCHECK(glEnable(GL_TEXTURE_2D));

      OGLCHECK(glEnable(GL_DEPTH_TEST));
      OGLCHECK(glDepthFunc(GL_LEQUAL));

      OGLCHECK(glEnable(GL_ALPHA_TEST));
      OGLCHECK(glAlphaFunc(GL_GREATER, 0));

      OGLCHECK(glEnable(GL_BLEND));
      OGLCHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

      OGLCHECK(glColor4f(1.0f, 1.0f, 1.0f, 1.0f));
*/
    }

    void ThreadRenderer::prepareBackBuffer()
    {
/*      threads::MutexGuard guard(*m_renderState->m_mutex.get());

      m_blitter->frameBuffer()->setRenderTarget(m_renderState->m_backBuffer);
      m_blitter->beginFrame();

      m_blitter->enableClipRect(true);
      m_blitter->setClipRect(m2::RectI(0, 0, m_renderState->m_textureWidth, m_renderState->m_textureHeight));
      m_blitter->clear();

      m_blitter->setClipRect(m2::RectI(0, 0, m_renderState->m_surfaceWidth, m_renderState->m_surfaceHeight));

      if (m_renderState->isPanning())
        m_blitter->blit(
            m_renderState->m_actualTarget,
            m_renderState->m_actualScreen,
            m_renderState->m_currentScreen
            );

      m_blitter->endFrame();
*/
    }

    void ThreadRenderer::beginFrame()
    {
//      m_blitter->beginFrame();
    }

    void ThreadRenderer::endFrame()
    {
//      m_blitter->endFrame();
    }

    void ThreadRenderer::setCurrentScreen(ScreenBase const & /*currentScreen*/)
    {
/*      LOG(LINFO, ("setCurrentScreen"));
      /// this prevents the framework from flooding us with a bunch of RenderCommands with the same screen.
      {
        threads::MutexGuard guard(*m_renderState->m_mutex.get());
        m_renderState->m_currentScreen = currentScreen;
      }
*/
    }

    void ThreadRenderer::processResize()
    {
/*      LOG(LINFO, ("processResize"));
      threads::MutexGuard guard(*m_renderState->m_mutex.get());

      if (m_renderState->m_isResized)
      {
        size_t texW = m_renderState->m_textureWidth;
        size_t texH = m_renderState->m_textureHeight;

        m_renderState->m_backBuffer.reset();
        m_renderState->m_backBuffer = make_shared_ptr(new yg::gl::RGBA8Texture(texW, texH));

        m_renderState->m_depthBuffer.reset();
        m_renderState->m_depthBuffer = make_shared_ptr(new yg::gl::RenderBuffer(texW, texH, true));

        m_blitter->setRenderTarget(m_renderState->m_backBuffer);
        m_blitter->setDepthBuffer(m_renderState->m_depthBuffer);

        m_blitter->onSize(texW, texH);

        m_renderState->m_actualTarget.reset();
        m_renderState->m_actualTarget = make_shared_ptr(new yg::gl::RGBA8Texture(texW, texH));

        m_blitter->setRenderTarget(m_renderState->m_actualTarget);
        m_blitter->beginFrame();
        m_blitter->clear();
        m_blitter->endFrame();

        m_blitter->setRenderTarget(m_renderState->m_backBuffer);
        m_blitter->beginFrame();
        m_blitter->clear();
        m_blitter->endFrame();

        m_renderState->m_doRepaintAll = true;

        m_renderState->m_isResized = false;
      }
*/
    }

    void ThreadRenderer::setViewport(m2::RectI const & /*viewport*/)
    {
/*      LOG(LINFO, ("setViewport:", viewport));
      if (m_viewport != viewport)
      {
        m_viewport = viewport;
        m_blitter->setClipRect(m_viewport);
//        OGLCHECK(glScissor(m_viewport.minX(), m_viewport.minY(), m_viewport.maxX(), m_viewport.maxY()));
//        m_viewport = viewport;
      }
*/
    }

    void ThreadRenderer::blitBackBuffer()
    {
/*      LOG(LINFO, ("blitBackBuffer"));
      m_blitter->enableClipRect(true);
      m_blitter->setClipRect(m2::RectI(0, 0, m_renderStateCopy.m_textureWidth, m_renderStateCopy.m_textureHeight));
      m_blitter->clear();

      m_blitter->setClipRect(m2::RectI(0, 0, m_renderStateCopy.m_surfaceWidth, m_renderStateCopy.m_surfaceHeight));

      if (m_renderStateCopy.isPanning())
        m_blitter->blit(
            m_renderStateCopy.m_actualTarget,
            m_renderStateCopy.m_actualScreen,
            m_renderStateCopy.m_currentScreen);
 */
    }

    void ThreadRenderer::drawGeometry(shared_ptr<BaseTexture> const & /*texture*/,
                                      shared_ptr<VertexBuffer> const & /*vertices*/,
                                      shared_ptr<IndexBuffer> const & /*indices*/,
                                      size_t /*indicesCount*/)
    {
/*      LOG(LINFO, ("drawGeometry"));

      if (m_texture != texture)
      {
        m_texture = texture;
        m_texture->makeCurrent();
      }

      vertices->makeCurrent();
      Vertex::setupLayout();

      indices->makeCurrent();

      OGLCHECK(glDrawElements(
        GL_TRIANGLES,
        indicesCount,
        GL_UNSIGNED_SHORT,
        0));

      updateActualTarget();
*/
    }

    void ThreadRenderer::updateActualTarget()
    {
/*      LOG(LINFO, ("updateActualTarget"));
      OGLCHECK(glFinish());

      threads::MutexGuard guard(*m_renderState->m_mutex.get());

      m_blitter->frameBuffer()->attachTexture(m_renderState->m_actualTarget);

      OGLCHECK(glDisable(GL_SCISSOR_TEST));

      OGLCHECK(glClearColor(192 / 255.0, 192 / 255.0, 192 / 255.0, 1.0));
      OGLCHECK(glClear(GL_COLOR_BUFFER_BIT));

      size_t w = m_renderState->m_backBuffer->width();
      size_t h = m_renderState->m_backBuffer->height();

      m_blitter->immDrawTexturedRect(
          m2::RectF(0, 0, w, h),
          m2::RectF(0, 0, 1, 1),
          m_renderState->m_backBuffer
          );

      if (m_blitter->clipRectEnabled())
        OGLCHECK(glEnable(GL_SCISSOR_TEST));

      OGLCHECK(glFinish());

      m_blitter->frameBuffer()->attachTexture(m_renderState->m_backBuffer);
*/
    }

    void ThreadRenderer::copyRenderState()
    {
/*      LOG(LINFO, ("copyRenderState"));
      m_renderState->copyTo(m_renderStateCopy);
 */
    }
  }
}
