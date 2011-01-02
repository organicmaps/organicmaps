#pragma once

#include "widgets.hpp"

#include "../yg/texture.hpp"
#include "../yg/resource_manager.hpp"
#include "../yg/renderbuffer.hpp"
#include "../map/drawer_yg.hpp"
#include "../map/qgl_render_context.hpp"

#include "../std/shared_ptr.hpp"


namespace yg
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
  class GLDrawWidget : public qt::GLDrawWidgetT<yg::gl::Screen>
  {
  protected:
    typedef qt::GLDrawWidgetT<yg::gl::Screen> base_type;

    shared_ptr<yg::ResourceManager> m_resourceManager;

    shared_ptr<yg::gl::FrameBuffer> m_primaryFrameBuffer;
    shared_ptr<yg::gl::FrameBuffer> m_frameBuffer;
    shared_ptr<yg::gl::RGBA8Texture> m_renderTarget;
    shared_ptr<yg::gl::RenderBuffer> m_depthBuffer;
    shared_ptr<yg::Skin> m_skin;
    shared_ptr<qt::gl::RenderContext> m_primaryContext;
    shared_ptr<yg::gl::Screen> m_primaryScreen;

  public:

    GLDrawWidget();
    virtual ~GLDrawWidget();

  protected:
    virtual void initializeGL();
    virtual void resizeGL(int w, int h);
    virtual void paintGL();
  };
}
