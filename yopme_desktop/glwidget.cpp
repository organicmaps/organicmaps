#include "glwidget.hpp"

#include "../map/qgl_render_context.hpp"
#include "../map/simple_render_policy.hpp"
#include "../map/yopme_render_policy.hpp"
#include "../graphics/render_context.hpp"
#include "../graphics/render_context.hpp"
#include "../platform/platform.hpp"

GLWidget::GLWidget(QWidget * parent)
  : QGLWidget(parent)
{
  m_f.AddLocalMaps();
}

GLWidget::~GLWidget()
{
  m_f.PrepareToShutdown();
}

void GLWidget::initializeGL()
{
  shared_ptr<graphics::RenderContext> primaryRC(new qt::gl::RenderContext(this));
  graphics::ResourceManager::Params rmParams;
  rmParams.m_rtFormat = graphics::Data8Bpp;
  rmParams.m_texFormat = graphics::Data8Bpp;
  rmParams.m_texRtFormat = graphics::Data8Bpp;
  rmParams.m_videoMemoryLimit = GetPlatform().VideoMemoryLimit();

  RenderPolicy::Params rpParams;

  rpParams.m_videoTimer = &m_timer;
  rpParams.m_useDefaultFB = true;
  rpParams.m_rmParams = rmParams;
  rpParams.m_primaryRC = primaryRC;
  rpParams.m_density = graphics::EDensityMDPI;
  rpParams.m_skinName = "basic.skn";
  rpParams.m_screenHeight = 640;
  rpParams.m_screenWidth = 340;

  try
  {
    m_f.SetRenderPolicy(new YopmeRP(rpParams));
    m_f.InitGuiSubsystem();
  }
  catch (RootException & e)
  {
    LOG(LERROR, (e.what()));
  }
}

void GLWidget::resizeGL(int w, int h)
{
  m_f.OnSize(w, h);
  m_f.Invalidate();
  updateGL();
}

void GLWidget::paintGL()
{
  m_f.GetNavigator().SetFromRect(m2::AnyRectD(
                             m2::RectD(MercatorBounds::LonToX(26.90),
                                       MercatorBounds::LatToY(53.42),
                                       MercatorBounds::LonToX(27.80),
                                       MercatorBounds::LatToY(54.64))));


  shared_ptr<PaintEvent> e(new PaintEvent(m_f.GetRenderPolicy()->GetDrawer().get()));
  m_f.BeginPaint(e);
  m_f.DoPaint(e);
  m_f.EndPaint(e);
}
