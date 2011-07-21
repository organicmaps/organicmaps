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
#ifdef OMIM_GL_ES
      OGLCHECK(glGetIntegerv(GL_FRAMEBUFFER_BINDING_OES, &id));
#else
      OGLCHECK(glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &id));
#endif
      return id;
    }

    FrameBuffer::FrameBuffer(bool defaultFB /*= false*/) : m_width(0), m_height(0)
    {
      if (defaultFB)
        m_id = 0;
      else
      {
#ifdef OMIM_GL_ES
        OGLCHECK(glGenFramebuffersOES(1, &m_id));
#else
        OGLCHECK(glGenFramebuffers(1, &m_id));
#endif
      }
    }

    FrameBuffer::~FrameBuffer()
    {
      if (m_id != 0)
      {
#ifdef OMIM_GL_ES
        OGLCHECK(glDeleteFramebuffersOES(1, &m_id));
#else
        OGLCHECK(glDeleteFramebuffers(1, &m_id));
#endif
      }
    }

    void FrameBuffer::makeCurrent()
    {
#ifndef OMIM_OS_ANDROID
      if (m_id != current())
#endif
      {
#ifdef OMIM_GL_ES
        OGLCHECK(glBindFramebufferOES(GL_FRAMEBUFFER_OES, m_id));
#else
        OGLCHECK(glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_id));
#endif
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
#ifdef OMIM_GL_ES
      GLenum res = glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES);
      if (res == GL_FRAMEBUFFER_UNSUPPORTED_OES)
        LOG(LINFO, ("unsupported combination of attached target formats. could be possibly skipped"));
      else if (res == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES)
        LOG(LINFO, ("incomplete attachement"));
      else if (res == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_OES)
        LOG(LINFO, ("incomplete missing attachement"));
      else if (res == GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_OES)
        LOG(LINFO, ("incomplete dimensions"));
#else
      GLenum res = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
      if (res == GL_FRAMEBUFFER_UNSUPPORTED)
      {
        LOG(LINFO, ("unsupported combination of attached target formats. could be possibly skipped"));
      }
      else if (res != GL_FRAMEBUFFER_COMPLETE_EXT)
      {
        LOG(LERROR, ("incomplete framebuffer"));
      }
#endif
    }
  }
}
