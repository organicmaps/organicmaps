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

    void RenderBuffer::checkID() const
    {
      if (!m_hasID)
      {
        m_hasID = true;
        OGLCHECK(glGenRenderbuffersFn(1, &m_id));
        makeCurrent();

        GLenum target = GL_RENDERBUFFER_MWM;
        GLenum internalFormat = m_isDepthBuffer ? GL_DEPTH_COMPONENT24_MWM : GL_RGBA8_MWM;

        OGLCHECK(glRenderbufferStorageFn(target,
                                         internalFormat,
                                         m_width,
                                         m_height));
      }
    }

    RenderBuffer::RenderBuffer(size_t width, size_t height, bool isDepthBuffer)
      : m_hasID(false), m_id(0), m_isDepthBuffer(isDepthBuffer), m_width(width), m_height(height)
    {}

    RenderBuffer::~RenderBuffer()
    {
      if ((m_hasID) && (g_doDeleteOnDestroy))
      {
        OGLCHECK(glDeleteRenderbuffersFn(1, &m_id));
      }
    }

    unsigned int RenderBuffer::id() const
    {
      checkID();
      return m_id;
    }

    void RenderBuffer::attachToFrameBuffer()
    {
      checkID();

      OGLCHECK(glFramebufferRenderbufferFn(
          GL_FRAMEBUFFER_MWM,
          isDepthBuffer() ? GL_DEPTH_ATTACHMENT_MWM : GL_COLOR_ATTACHMENT0_MWM,
          GL_RENDERBUFFER_MWM,
          id()));

      if (!isDepthBuffer())
        utils::setupCoordinates(width(), height(), false);
    }

    void RenderBuffer::makeCurrent() const
    {
      checkID();
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
