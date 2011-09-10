#include "../base/SRC_FIRST.hpp"
#include "renderer.hpp"
#include "utils.hpp"
#include "framebuffer.hpp"
#include "renderbuffer.hpp"
#include "resource_manager.hpp"
#include "internal/opengl.hpp"
#include "../base/logging.hpp"

namespace yg
{
  namespace gl
  {
    Renderer::Params::Params() : m_isDebugging(false)
    {}

    Renderer::Renderer(Params const & params)
      : m_frameBuffer(params.m_frameBuffer),
        m_isDebugging(params.m_isDebugging),
        m_isRendering(false)
    {
      m_resourceManager = params.m_resourceManager;
    }

    shared_ptr<ResourceManager> const & Renderer::resourceManager() const
    {
      return m_resourceManager;
    }

    void Renderer::beginFrame()
    {
      m_isRendering = true;
      if (m_frameBuffer.get() != 0)
        m_frameBuffer->makeCurrent();
    }

    bool Renderer::isRendering() const
    {
      return m_isRendering;
    }

    void Renderer::endFrame()
    {
        m_isRendering = false;
    }

    shared_ptr<FrameBuffer> const & Renderer::frameBuffer() const
    {
      return m_frameBuffer;
    }

    shared_ptr<RenderTarget> const & Renderer::renderTarget() const
    {
      return m_frameBuffer->renderTarget();
    }

    void Renderer::setRenderTarget(shared_ptr<RenderTarget> const & rt)
    {
      m_frameBuffer->setRenderTarget(rt);
      m_frameBuffer->makeCurrent(); //< to attach renderTarget
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

    void Renderer::finish()
    {
      OGLCHECK(glFinish());
    }

    void Renderer::onSize(unsigned int width, unsigned int height)
    {
      if (width < 2) width = 2;
      if (height < 2) height = 2;

      m_width = width;
      m_height = height;

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

    bool Renderer::isDebugging() const
    {
      return m_isDebugging;
    }
  }
}
