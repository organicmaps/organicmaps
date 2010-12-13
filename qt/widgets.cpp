#include "widgets.hpp"

#include "../qt_tstfrm/widgets_impl.hpp"

#include "../map/drawer_yg.hpp"

#include "../platform/platform.hpp"

#include "../base/ptr_utils.hpp"

#include "../yg/rendercontext.hpp"

#ifdef WIN32
#include "../yg/internal/opengl_win32.hpp"
#endif

#include "../base/start_mem_debug.hpp"

namespace qt
{
  template class GLDrawWidgetT<DrawerYG>;

  GLDrawWidget::GLDrawWidget(QWidget * pParent)
    : base_type(pParent)
  {
  }

  void GLDrawWidget::initializeGL()
  {
    if (m_p == 0)
    {
#ifdef WIN32
      win32::InitOpenGL();
#endif
      m_renderContext = shared_ptr<yg::gl::RenderContext>(new qt::gl::RenderContext(this));
      m_resourceManager = make_shared_ptr(new yg::ResourceManager(
          50000 * sizeof(yg::gl::Vertex),
          100000 * sizeof(unsigned short),
          15,
          5000 * sizeof(yg::gl::Vertex),
          10000 * sizeof(unsigned short),
          100,
          512, 256,
          15));

      m_resourceManager->addFont(GetPlatform().ReadPathForFile("dejavusans.ttf").c_str());

      m_p = shared_ptr<DrawerYG>(new DrawerYG(m_resourceManager, GetPlatform().SkinName(), !GetPlatform().IsMultiSampled()));
      m_p->setFrameBuffer(make_shared_ptr(new yg::gl::FrameBuffer(true)));
    }
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
