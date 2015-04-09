#pragma once

#include "graphics/opengl/gl_render_context.hpp"

#include "std/shared_ptr.hpp"

class QWidget;
class QGLContext;
class QGLWidget;

namespace qt
{
  namespace gl
  {
    class RenderContext : public graphics::gl::RenderContext
    {
    private:
      shared_ptr<QGLContext> m_context;
      shared_ptr<QWidget> m_parent;

      /// Creates a rendering context, which shares
      /// data(textures, display lists e.t.c) with renderContext.
      /// Used in rendering thread.
      RenderContext(RenderContext * renderContext);

    public:
      RenderContext(QGLWidget * widget);
      ~RenderContext();

      /// Make this rendering context current
      void makeCurrent();

      graphics::RenderContext * createShared();

      /// Leave previous logic, but fix thread widget deletion error.
      void endThreadDrawing(unsigned threadSlot);

      shared_ptr<QGLContext> context() const;
    };
  }
}
