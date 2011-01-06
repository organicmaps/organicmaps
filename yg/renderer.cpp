#include "../base/SRC_FIRST.hpp"
#include "renderer.hpp"
#include "utils.hpp"
#include "framebuffer.hpp"
#include "renderbuffer.hpp"
#include "resource_manager.hpp"
#include "../base/ptr_utils.hpp"
#include "internal/opengl.hpp"

namespace yg
{
  namespace gl
  {
    Renderer::Params::Params() : m_isMultiSampled(false)
    {}

    Renderer::Renderer(Params const & params)
      : m_frameBuffer(params.m_frameBuffer),
        m_isMultiSampled(params.m_isMultiSampled),
        m_isRendering(false)
    {
      if (m_isMultiSampled)
        m_multiSampledFrameBuffer = make_shared_ptr(new FrameBuffer());
      m_resourceManager = params.m_resourceManager;
    }

    shared_ptr<ResourceManager> const & Renderer::resourceManager() const
    {
      return m_resourceManager;
    }

    void Renderer::beginFrame()
    {
      m_isRendering = true;
      if (m_isMultiSampled)
        m_multiSampledFrameBuffer->makeCurrent();
      else
        if (m_frameBuffer.get() != 0)
          m_frameBuffer->makeCurrent();
    }

    bool Renderer::isRendering() const
    {
      return m_isRendering;
    }

    void Renderer::endFrame()
    {
//      if (m_isMultiSampled)
        updateFrameBuffer();
        m_isRendering = false;
    }

    shared_ptr<FrameBuffer> const & Renderer::frameBuffer() const
    {
      return m_frameBuffer;
    }

    shared_ptr<FrameBuffer> const & Renderer::multiSampledFrameBuffer() const
    {
      return m_multiSampledFrameBuffer;
    }

/*    void Renderer::setFrameBuffer(shared_ptr<FrameBuffer> const & fb)
    {
      m_frameBuffer = fb;
    }*/

    shared_ptr<RenderTarget> const & Renderer::renderTarget() const
    {
      return m_frameBuffer->renderTarget();
    }

    void Renderer::setRenderTarget(shared_ptr<RenderTarget> const & rt)
    {
      if (isRendering())
        updateFrameBuffer();

      m_frameBuffer->setRenderTarget(rt);
      m_frameBuffer->makeCurrent(); //< to attach renderTarget

      if (m_isMultiSampled)
        m_multiSampledFrameBuffer->makeCurrent();
    }


    void Renderer::updateFrameBuffer()
    {
      if (m_isMultiSampled)
      {
#ifdef OMIM_GL_ES

        OGLCHECK(glBindFramebufferOES(GL_READ_FRAMEBUFFER_APPLE, m_multiSampledFrameBuffer->id()));
        OGLCHECK(glBindFramebufferOES(GL_DRAW_FRAMEBUFFER_APPLE, m_frameBuffer->id()));
        OGLCHECK(glResolveMultisampleFramebufferAPPLE());
        OGLCHECK(glBindFramebufferOES(GL_DRAW_FRAMEBUFFER_APPLE, m_multiSampledFrameBuffer->id()));

#else
        /// Somehow this does the trick with the "trash-texture" upon first application redraw.
        m_multiSampledFrameBuffer->makeCurrent();

        OGLCHECK(glBindFramebuffer(GL_READ_FRAMEBUFFER, m_multiSampledFrameBuffer->id()));
        OGLCHECK(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_frameBuffer->id()));
        OGLCHECK(glBlitFramebuffer(0, 0, width(), height(),
                                   0, 0, width(), height(),
                                   GL_COLOR_BUFFER_BIT,
                                   GL_NEAREST));
        OGLCHECK(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_multiSampledFrameBuffer->id()));

#endif
        OGLCHECK(glFinish());
      }
    }

    bool Renderer::isMultiSampled() const
    {
      return m_isMultiSampled;
    }

    void Renderer::clear(yg::Color const & c, bool clearRT, float depth, bool clearDepth)
    {
      OGLCHECK(glClearColor(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f));
 #ifdef OMIM_GL_ES
      OGLCHECK(glClearDepthf(depth));
 #else
      OGLCHECK(glClearDepth(depth));
 #endif

      GLbitfield mask = 0;
      if (clearRT)
        mask |= GL_COLOR_BUFFER_BIT;
      if (clearDepth)
        mask |= GL_DEPTH_BUFFER_BIT;

      OGLCHECK(glClear(mask));
    }

    void Renderer::onSize(unsigned int width, unsigned int height)
    {
      if (width < 2) width = 2;
      if (height < 2) height = 2;

      m_width = width;
      m_height = height;

      if (m_isMultiSampled)
      {
        m_multiSampledFrameBuffer->onSize(m_width, m_height);
        if ( (!m_multiSampledRenderTarget.get())
          || (m_multiSampledRenderTarget->width() != width)
          || (m_multiSampledRenderTarget->height() != height) )
          {
            m_multiSampledRenderTarget.reset();
            m_multiSampledRenderTarget = make_shared_ptr(new RenderBuffer(width, height, false, true));
            m_multiSampledFrameBuffer->setRenderTarget(m_multiSampledRenderTarget);

            m_multiSampledDepthBuffer.reset();
            m_multiSampledDepthBuffer = make_shared_ptr(new RenderBuffer(width, height, true, true));
            m_multiSampledFrameBuffer->setDepthBuffer(m_multiSampledDepthBuffer);
          }
      }

      if (m_frameBuffer)
        m_frameBuffer->onSize(width, height);
    }

    unsigned int Renderer::width() const
    {
      return m_width;
    }

    unsigned int Renderer::height() const
    {
      return m_height;
    }

  }
}
