#include "graphics/opengl/renderbuffer.hpp"
#include "graphics/opengl/opengl.hpp"
#include "graphics/opengl/utils.hpp"

#include "base/logging.hpp"
#include "base/assert.hpp"

#include "std/list.hpp"

namespace graphics
{
  namespace gl
  {
    RenderBuffer::RenderBuffer(size_t width, size_t height, bool isDepthBuffer, bool isRgba4)
      : m_id(0), m_isDepthBuffer(isDepthBuffer), m_width(width), m_height(height)
    {
      OGLCHECK(glGenRenderbuffersFn(1, &m_id));

      makeCurrent();

      GLenum target = GL_RENDERBUFFER_MWM;
      GLenum internalFormat = m_isDepthBuffer ? GL_DEPTH_COMPONENT16_MWM : GL_RGBA8_MWM;

      if (m_isDepthBuffer == false && isRgba4 == true)
        internalFormat = GL_RGBA4_MWM;

      if (m_isDepthBuffer == false)
        LOG(LDEBUG, ("Color buffer format : ", internalFormat));
      OGLCHECK(glRenderbufferStorageFn(target,
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
