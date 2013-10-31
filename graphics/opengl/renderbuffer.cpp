#include "renderbuffer.hpp"
#include "opengl.hpp"
#include "utils.hpp"

#include "../../base/logging.hpp"
#include "../../base/assert.hpp"

#include "../../std/list.hpp"

namespace graphics
{
  namespace gl
  {
    RenderBuffer::RenderBuffer(size_t width, size_t height)
      : m_id(0), m_isDepthBuffer(true), m_width(width), m_height(height)
    {
      OGLCHECK(glGenRenderbuffersFn(1, &m_id));

      makeCurrent();

      OGLCHECK(glRenderbufferStorageFn(GL_RENDERBUFFER_MWM,
                                       GL_DEPTH_COMPONENT16_MWM,
                                       m_width,
                                       m_height));
    }

    RenderBuffer::RenderBuffer(size_t width, size_t height, graphics::DataFormat format)
      : m_id(0), m_isDepthBuffer(false), m_width(width), m_height(height)
    {
      OGLCHECK(glGenRenderbuffersFn(1, &m_id));

      makeCurrent();

      GLenum internalFormat;
      switch(format)
      {
      case graphics::Data4Bpp:
        internalFormat = GL_RGBA4_MWM;
        break;
      case graphics::Data8Bpp:
        internalFormat = GL_RGBA8_MWM;
        break;
      default:
        ASSERT(false, ("Incorrect color buffer format"));
      }

      OGLCHECK(glRenderbufferStorageFn(GL_RENDERBUFFER_MWM,
                                       internalFormat,
                                       m_width,
                                       m_height));
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
