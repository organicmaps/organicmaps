/*#include "widgets.hpp"

#include "../qt_tstfrm/widgets_impl.hpp"

#include "../map/drawer.hpp"

#include "../platform/platform.hpp"

#include "../graphics/rendercontext.hpp"
#include "../graphics/internal/opengl.hpp"


namespace qt
{
  template class GLDrawWidgetT<Drawer>;

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

  shared_ptr<graphics::gl::RenderContext> const & GLDrawWidget::renderContext()
  {
    return m_renderContext;
  }

  shared_ptr<graphics::ResourceManager> const & GLDrawWidget::resourceManager()
  {
    return m_resourceManager;
  }
}
*/
