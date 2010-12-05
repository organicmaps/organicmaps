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
#ifdef OMIM_GL_ES
      OGLCHECK(glGetIntegerv(GL_RENDERBUFFER_BINDING_OES, &id));
#else
      OGLCHECK(glGetIntegerv(GL_RENDERBUFFER_BINDING_EXT, &id));
#endif
      return id;
    }

    void RenderBuffer::pushCurrent()
    {
      renderBufferStack.push_back(current());
    }

    void RenderBuffer::popCurrent()
    {
#ifdef OMIM_GL_ES
        OGLCHECK(glBindRenderbufferOES(GL_RENDERBUFFER_OES, renderBufferStack.back()));
#else
        OGLCHECK(glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, renderBufferStack.back()));
#endif
        renderBufferStack.pop_back();
    }

    RenderBuffer::RenderBuffer(size_t width, size_t height, bool isDepthBuffer, bool isMultiSampled)
      : m_isDepthBuffer(isDepthBuffer), m_isMultiSampled(isMultiSampled), m_width(width), m_height(height)
    {
        pushCurrent();
#ifdef OMIM_GL_ES
        OGLCHECK(glGenRenderbuffersOES(1, &m_id));
        makeCurrent();

        GLenum target = GL_RENDERBUFFER_OES;
        GLenum internalFormat = m_isDepthBuffer ? GL_DEPTH_COMPONENT16_OES : GL_RGBA8_OES;

        if (m_isMultiSampled)
          OGLCHECK(glRenderbufferStorageMultisampleAPPLE(target,
                                                         4,
                                                         internalFormat,
                                                         width,
                                                         height));
        else
          OGLCHECK(glRenderbufferStorageOES(GL_RENDERBUFFER_OES,
                                            m_isDepthBuffer ? GL_DEPTH_COMPONENT16_OES : GL_RGBA8_OES,
                                            width,
                                            height));
#else
        OGLCHECK(glGenRenderbuffers(1, &m_id));
        makeCurrent();

        GLenum target = GL_RENDERBUFFER_EXT;
        GLenum internalFormat = m_isDepthBuffer ? GL_DEPTH_COMPONENT16 : GL_RGBA;

        if (m_isMultiSampled)
          OGLCHECK(glRenderbufferStorageMultisample(target,
                                                    4,
                                                    internalFormat,
                                                    width,
                                                    height));
        else
          OGLCHECK(glRenderbufferStorageEXT(target,
                                            m_isDepthBuffer ? GL_DEPTH_COMPONENT16 : GL_RGBA,
                                            width,
                                            height));

#endif
        popCurrent();
    }

    RenderBuffer::~RenderBuffer()
    {
      if (m_id != 0)
      {
#ifdef OMIM_GL_ES
        OGLCHECK(glDeleteRenderbuffersOES(1, &m_id));
#else
        OGLCHECK(glDeleteRenderbuffersEXT(1, &m_id));
#endif
      }
    }

    unsigned int RenderBuffer::id() const
    {
      return m_id;
    }

    void RenderBuffer::attachToFrameBuffer()
    {
#ifdef OMIM_GL_ES
      OGLCHECK(glFramebufferRenderbufferOES(
          GL_FRAMEBUFFER_OES,
          isDepthBuffer() ? GL_DEPTH_ATTACHMENT_OES : GL_COLOR_ATTACHMENT0_OES,
          GL_RENDERBUFFER_OES,
          id()));
#else
      OGLCHECK(glFramebufferRenderbuffer(
          GL_FRAMEBUFFER_EXT,
          isDepthBuffer() ? GL_DEPTH_ATTACHMENT_EXT : GL_COLOR_ATTACHMENT0_EXT,
          GL_RENDERBUFFER_EXT,
          id()));
#endif
      if (!isDepthBuffer())
        utils::setupCoordinates(width(), height(), false);
    }

    void RenderBuffer::makeCurrent()
    {
      if (m_id != current())
      {
#ifdef OMIM_GL_ES
        OGLCHECK(glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_id));
#else
        OGLCHECK(glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, m_id));
#endif
      }
    }

    bool RenderBuffer::isDepthBuffer() const
    {
      return m_isDepthBuffer;
    }

    bool RenderBuffer::isMultiSampled() const
    {
      return m_isMultiSampled;
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
