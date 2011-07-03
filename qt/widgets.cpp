#include "widgets.hpp"

#include "../qt_tstfrm/widgets_impl.hpp"

#include "../map/drawer_yg.hpp"

#include "../platform/platform.hpp"

#include "../yg/rendercontext.hpp"
#include "../yg/internal/opengl.hpp"

#ifdef OMIM_OS_WINDOWS
  #include "../yg/internal/opengl_win32.hpp"
#endif

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

      if (!yg::gl::CheckExtensionSupport())
      {
        /// TODO: Show "Please Update Drivers" dialog and close the program.
      }

      Platform & pl = GetPlatform();

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
          10,
          512, 256,
          5,
          256, 256, 80,
          "unicode_blocks.txt",
          "fonts_whitelist.txt",
          "fonts_blacklist.txt",
          2 * 1024 * 1024,
          yg::Rt8Bpp,
          !yg::gl::g_isBufferObjectsSupported,
          !pl.IsMultiSampled()));

      Platform::FilesList fonts;
      pl.GetFontNames(fonts);
      m_resourceManager->addFonts(fonts);

      DrawerYG::params_t p;

      p.m_resourceManager = m_resourceManager;
      p.m_isMultiSampled = false;
      p.m_frameBuffer = make_shared_ptr(new yg::gl::FrameBuffer(true));
      p.m_dynamicPagesCount = 2;
      p.m_textPagesCount = 2;
      p.m_glyphCacheID = m_resourceManager->guiThreadGlyphCacheID();

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
