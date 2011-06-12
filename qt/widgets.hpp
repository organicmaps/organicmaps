#pragma once

#include "../qt_tstfrm/widgets.hpp"
#include "../map/qgl_render_context.hpp"
#include "../yg/resource_manager.hpp"

class DrawerYG;

namespace yg
{
  namespace gl
  {
    class RenderContext;
  }
}

namespace qt
{
  /// Widget uses yg for drawing.
  class GLDrawWidget : public GLDrawWidgetT<DrawerYG>
  {
    typedef GLDrawWidgetT<DrawerYG> base_type;
    shared_ptr<yg::gl::RenderContext> m_renderContext;

  protected:
    shared_ptr<yg::ResourceManager> m_resourceManager;

  public:
    typedef DrawerYG drawer_t;

    GLDrawWidget(QWidget * pParent);

    shared_ptr<yg::gl::RenderContext> const & renderContext();
    shared_ptr<yg::ResourceManager> const & resourceManager();

  protected:
    virtual void initializeGL();
  };
}
