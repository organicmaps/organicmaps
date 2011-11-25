#include "../base/SRC_FIRST.hpp"

#include "framebuffer.hpp"
#include "render_target.hpp"
#include "internal/opengl.hpp"
#include "utils.hpp"

#include "../base/logging.hpp"
#include "../std/list.hpp"

namespace yg
{
  namespace gl
  {
    list<unsigned int> frameBufferStack;

    unsigned FrameBuffer::current()
    {
      int id;
      OGLCHECK(glGetIntegerv(GL_FRAMEBUFFER_BINDING_MWM, &id));
      return id;
    }

    FrameBuffer::FrameBuffer(bool defaultFB /*= false*/) : m_width(0), m_height(0)
    {
      if (defaultFB)
        m_id = 0;
      else
        OGLCHECK(glGenFramebuffersFn(1, &m_id));
    }

    FrameBuffer::~FrameBuffer()
    {
      if ((m_id != 0) && g_doDeleteOnDestroy)
        OGLCHECK(glDeleteFramebuffersFn(1, &m_id));
    }

    void FrameBuffer::makeCurrent()
    {
#ifndef OMIM_OS_ANDROID
      if (m_id != current())
#endif
      {
        OGLCHECK(glBindFramebufferFn(GL_FRAMEBUFFER_MWM, m_id));
//        LOG(LINFO, ("FrameBuffer::makeCurrent", m_id));
      }

      if (m_renderTarget)
        m_renderTarget->attachToFrameBuffer();
      else
        utils::setupCoordinates(width(), height(), true);
      if (m_depthBuffer)
        m_depthBuffer->attachToFrameBuffer();

      /// !!! it's a must for a correct work.
      checkStatus();
    }

    void FrameBuffer::setRenderTarget(shared_ptr<RenderTarget> const & renderTarget)
    {
      m_renderTarget = renderTarget;
    }

    shared_ptr<RenderTarget> const & FrameBuffer::renderTarget() const
    {
      return m_renderTarget;
    }

    void FrameBuffer::resetRenderTarget()
    {
      m_renderTarget.reset();
    }

    void FrameBuffer::setDepthBuffer(shared_ptr<RenderTarget> const & depthBuffer)
    {
      m_depthBuffer = depthBuffer;
    }

    shared_ptr<RenderTarget> const & FrameBuffer::depthBuffer() const
    {
      return m_depthBuffer;
    }

    void FrameBuffer::resetDepthBuffer()
    {
      m_depthBuffer.reset();
    }

    int FrameBuffer::id() const
    {
      return m_id;
    }

    unsigned FrameBuffer::width() const
    {
      return m_width;
    }

    unsigned FrameBuffer::height() const
    {
      return m_height;
    }

    void FrameBuffer::onSize(unsigned width, unsigned height)
    {
      m_width = width;
      m_height = height;
    }

    void FrameBuffer::checkStatus()
    {
      GLenum res = glCheckFramebufferStatusFn(GL_FRAMEBUFFER_MWM);
      if (res == GL_FRAMEBUFFER_UNSUPPORTED_MWM)
        LOG(LINFO, ("unsupported combination of attached target formats. could be possibly skipped"));
      else if (res == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_MWM)
        LOG(LINFO, ("incomplete attachement"));
      else if (res == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_MWM)
        LOG(LINFO, ("incomplete missing attachement"));
      else if (res == GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_MWM)
      {
        LOG(LINFO, ("incomplete dimensions"));
      }
    }
  }
}
