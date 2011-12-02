#include "widgets.hpp"

#include "../qt_tstfrm/widgets_impl.hpp"

#include "../map/drawer_yg.hpp"

#include "../platform/platform.hpp"

#include "../yg/rendercontext.hpp"
#include "../yg/internal/opengl.hpp"


namespace qt
{
  template class GLDrawWidgetT<DrawerYG>;

  GLDrawWidget::GLDrawWidget(QWidget * pParent)
    : base_type(pParent)
  {
  }

  shared_ptr<GLDrawWidget::drawer_t> const & GLDrawWidget::GetDrawer() const
  {
    return m_p;
  }

  GLDrawWidget::~GLDrawWidget()
  {
    makeCurrent();
    m_p.reset();
    doneCurrent();
  }

  void GLDrawWidget::initializeGL()
  {
  }

  shared_ptr<yg::gl::RenderContext> const & GLDrawWidget::renderContext()
  {
    return m_renderContext;
  }

  shared_ptr<yg::ResourceManager> const & GLDrawWidget::resourceManager()
  {
    return m_resourceManager;
  }
}
