#include "widgets.hpp"

#include "../qt_tstfrm/widgets_impl.hpp"

#include "../map/drawer_yg.hpp"

#include "../platform/platform.hpp"

#include "../yg/rendercontext.hpp"

#ifdef OMIM_OS_WINDOWS
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
#ifdef OMIM_OS_WINDOWS
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
          10 * sizeof(yg::gl::AuxVertex),
          10 * sizeof(unsigned short),
          50,
          512, 256,
          15,
          GetPlatform().ReadPathForFile("unicode_blocks.txt").c_str(),
          GetPlatform().ReadPathForFile("fonts_whitelist.txt").c_str(),
          GetPlatform().ReadPathForFile("fonts_blacklist.txt").c_str(),
          2000000));

      m_resourceManager->addFonts(GetPlatform().GetFontNames());

      DrawerYG::params_t p;

      p.m_resourceManager = m_resourceManager;
      p.m_isMultiSampled = false;
      p.m_frameBuffer = make_shared_ptr(new yg::gl::FrameBuffer(true));
      p.m_dynamicPagesCount = 2;
      p.m_textPagesCount = 2;

      m_p = shared_ptr<DrawerYG>(new DrawerYG(GetPlatform().SkinName(), p));
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
