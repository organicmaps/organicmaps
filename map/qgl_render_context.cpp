#include "base/SRC_FIRST.hpp"

#include "map/qgl_render_context.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <QtOpenGL/QGLContext>
#include <QtOpenGL/QGLWidget>
#include <QtOpenGL/QGLFramebufferObject>


namespace qt
{
  namespace gl
  {
    struct null_deleter
    {
      template <typename T> void operator()(T*) {}
    };

    /// Create compatible render context
    RenderContext::RenderContext(QGLWidget * widget)
    {
      /// Dirty hack, but we'll use it with caution, I promise.
      m_context = shared_ptr<QGLContext>(const_cast<QGLContext*>(widget->context()), null_deleter());
    }

    void RenderContext::makeCurrent()
    {
      m_context->makeCurrent();
    }

    graphics::RenderContext * RenderContext::createShared()
    {
      graphics::gl::RenderContext * res = new RenderContext(this);
      res->setResourceManager(resourceManager());
      return res;
    }

    void RenderContext::endThreadDrawing(unsigned threadSlot)
    {
      m_context.reset();
      graphics::gl::RenderContext::endThreadDrawing(threadSlot);
    }

    RenderContext::RenderContext(RenderContext * renderContext)
    {
      QGLFormat const format = renderContext->context()->format();
      m_parent = make_shared<QWidget>();
      m_context = make_shared<QGLContext>(format, m_parent.get());
      bool sharedContextCreated = m_context->create(renderContext->context().get());
      bool isSharing = m_context->isSharing();
      ASSERT(sharedContextCreated && isSharing, ("cannot create shared opengl context"));
      if (!sharedContextCreated || !isSharing)
        m_context.reset();
    }

    RenderContext::~RenderContext()
    {
      m_context.reset();
      m_parent.reset();
    }

    shared_ptr<QGLContext> RenderContext::context() const
    {
      return m_context;
    }
  }
}
