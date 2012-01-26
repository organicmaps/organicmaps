#include "../base/SRC_FIRST.hpp"

#include "internal/opengl.hpp"
#include "renderbuffer.hpp"
#include "utils.hpp"

#include "../base/logging.hpp"

#include "../std/list.hpp"

namespace yg
{
  namespace gl
  {
    list<unsigned int> renderBufferStack;

    int RenderBuffer::current()
    {
      int id;
      OGLCHECK(glGetIntegerv(GL_RENDERBUFFER_BINDING_MWM, &id));
      return id;
    }

    RenderBuffer::RenderBuffer(size_t width, size_t height, bool isDepthBuffer)
      : m_id(0), m_isDepthBuffer(isDepthBuffer), m_width(width), m_height(height)
    {
      {
        OGLCHECK(glGenRenderbuffersFn(1, &m_id));

        makeCurrent();

        GLenum target = GL_RENDERBUFFER_MWM;
        GLenum internalFormat = m_isDepthBuffer ? GL_DEPTH_COMPONENT16_MWM : GL_RGBA8_MWM;

        OGLCHECK(glRenderbufferStorageFn(target,
                                         internalFormat,
                                         m_width,
                                         m_height));
      }
    }

    RenderBuffer::~RenderBuffer()
    {
      if (g_hasContext)
        OGLCHECK(glDeleteRenderbuffersFn(1, &m_id));
    }

    unsigned int RenderBuffer::id() const
    {
      return m_id;
    }

    void RenderBuffer::attachToFrameBuffer()
    {
      OGLCHECK(glFramebufferRenderbufferFn(
          GL_FRAMEBUFFER_MWM,
          isDepthBuffer() ? GL_DEPTH_ATTACHMENT_MWM : GL_COLOR_ATTACHMENT0_MWM,
          GL_RENDERBUFFER_MWM,
          id()));

      if (!isDepthBuffer())
        utils::setupCoordinates(width(), height(), false);
    }

    void RenderBuffer::detachFromFrameBuffer()
    {
      OGLCHECK(glFramebufferRenderbufferFn(
               GL_FRAMEBUFFER_MWM,
               isDepthBuffer() ? GL_DEPTH_ATTACHMENT_MWM : GL_COLOR_ATTACHMENT0_MWM,
               GL_RENDERBUFFER_MWM,
               0));
    }

    void RenderBuffer::makeCurrent() const
    {
#ifndef OMIM_OS_ANDROID
      if (m_id != current())
#endif
        OGLCHECK(glBindRenderbufferFn(GL_RENDERBUFFER_MWM, m_id));
    }

    bool RenderBuffer::isDepthBuffer() const
    {
      return m_isDepthBuffer;
    }

    unsigned RenderBuffer::width() const
    {
      return m_width;
    }

    unsigned RenderBuffer::height() const
    {
      return m_height;
    }
  }
}
