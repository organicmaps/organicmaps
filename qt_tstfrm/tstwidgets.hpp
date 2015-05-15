#pragma once

#include "graphics/opengl/texture.hpp"
#include "graphics/opengl/renderbuffer.hpp"
#include "graphics/opengl/framebuffer.hpp"

#include "graphics/screen.hpp"

#include "graphics/resource_manager.hpp"

#include "map/qgl_render_context.hpp"

#include "std/shared_ptr.hpp"

#include <QtOpenGL/QGLWidget>

namespace graphics
{
  namespace gl
  {
    class Screen;
  }
}

namespace tst
{
  class GLDrawWidget : public QGLWidget
  {
  public:

    shared_ptr<graphics::ResourceManager> m_resourceManager;

    shared_ptr<graphics::gl::FrameBuffer> m_primaryFrameBuffer;
    shared_ptr<qt::gl::RenderContext> m_primaryContext;
    shared_ptr<graphics::Screen> m_primaryScreen;

    virtual void initializeGL();
    virtual void resizeGL(int w, int h);
    virtual void paintGL();

  public:

    virtual void DoDraw(shared_ptr<graphics::Screen> const & screen) = 0;
  };
}
