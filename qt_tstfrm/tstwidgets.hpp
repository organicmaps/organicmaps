#pragma once

#include "widgets.hpp"

#include "../graphics/texture.hpp"
#include "../graphics/resource_manager.hpp"
#include "../graphics/renderbuffer.hpp"
#include "../map/drawer.hpp"
#include "../map/qgl_render_context.hpp"

#include "../std/shared_ptr.hpp"


namespace graphics
{
  class Skin;
  namespace gl
  {
    class Screen;
  }
}

namespace qt { class Screen; }

namespace tst
{
  class GLDrawWidget : public qt::GLDrawWidgetT<graphics::gl::Screen>
  {
  protected:
    typedef qt::GLDrawWidgetT<graphics::gl::Screen> base_type;

    shared_ptr<graphics::ResourceManager> m_resourceManager;

    shared_ptr<graphics::gl::FrameBuffer> m_primaryFrameBuffer;
    shared_ptr<graphics::gl::FrameBuffer> m_frameBuffer;
    shared_ptr<graphics::gl::RGBA8Texture> m_renderTarget;
    shared_ptr<graphics::gl::RenderBuffer> m_depthBuffer;
    shared_ptr<graphics::Skin> m_skin;
    shared_ptr<qt::gl::RenderContext> m_primaryContext;
    shared_ptr<graphics::gl::Screen> m_primaryScreen;

  public:

    GLDrawWidget();
    virtual ~GLDrawWidget();

  protected:
    virtual void initializeGL();
    virtual void resizeGL(int w, int h);
    virtual void paintGL();
  };
}
