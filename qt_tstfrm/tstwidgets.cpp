#include "tstwidgets.hpp"
#include "widgets_impl.hpp"
#include "screen_qt.hpp"

#include "../yg/screen.hpp"
#include "../yg/utils.hpp"
#include "../yg/skin.hpp"
#include "../yg/framebuffer.hpp"
#include "../yg/renderbuffer.hpp"
#include "../yg/resource_manager.hpp"

#ifdef WIN32
#include "../yg/internal/opengl_win32.hpp"
#endif

#include "../platform/platform.hpp"

#include "../base/start_mem_debug.hpp"
#include "../base/ptr_utils.hpp"


template class qt::GLDrawWidgetT<yg::gl::Screen>;

namespace tst {

GLDrawWidget::GLDrawWidget() : base_type(0)
{
}

GLDrawWidget::~GLDrawWidget()
{
}

void GLDrawWidget::initializeGL()
{
#ifdef WIN32
  win32::InitOpenGL();
#endif

  m_primaryContext = make_shared_ptr(new qt::gl::RenderContext(this));

  m_resourceManager = make_shared_ptr(new yg::ResourceManager(
      30000 * sizeof(yg::gl::Vertex),
      50000 * sizeof(unsigned short),
      20,
      3000 * sizeof(yg::gl::Vertex),
      5000 * sizeof(unsigned short),
      100,
      512, 256, 15,
      2000000));

//  m_resourceManager->addFont(GetPlatform().ReadPathForFile("dejavusans.ttf").c_str());
  m_resourceManager->addFont(GetPlatform().ReadPathForFile("wqy-microhei.ttf").c_str());
/*  m_resourceManager = make_shared_ptr(new yg::ResourceManager(
      5000 * sizeof(yg::gl::Vertex),
      10000 * sizeof(unsigned short),
      20,
      256, 256, 10));*/

  m_p = make_shared_ptr(new yg::gl::Screen(m_resourceManager));

  m_primaryFrameBuffer = make_shared_ptr(new yg::gl::FrameBuffer(true));

  m_frameBuffer = make_shared_ptr(new yg::gl::FrameBuffer());

  m_p->setFrameBuffer(m_frameBuffer);

  m_skin = shared_ptr<yg::Skin>(loadSkin(m_resourceManager, GetPlatform().SkinName()));
  m_p->setSkin(m_skin);
}

void GLDrawWidget::resizeGL(int w, int h)
{
  m_p->onSize(w, h);

  m_frameBuffer->onSize(w, h);
  m_primaryFrameBuffer->onSize(w, h);

  m_depthBuffer.reset();
  m_depthBuffer = make_shared_ptr(new yg::gl::RenderBuffer(w, h, true));
  m_frameBuffer->setDepthBuffer(m_depthBuffer);

  m_renderTarget.reset();
  m_renderTarget = make_shared_ptr(new yg::gl::RGBA8Texture(w, h));
  m_frameBuffer->setRenderTarget(m_renderTarget);
}

void GLDrawWidget::paintGL()
{
  base_type::paintGL();

  m_p->setFrameBuffer(m_primaryFrameBuffer);

  m_p->beginFrame();

  m_p->immDrawTexturedRect(
      m2::RectF(0, 0, m_renderTarget->width(), m_renderTarget->height()),
      m2::RectF(0, 0, 1, 1),
      m_renderTarget
      );

  m_p->endFrame();

  m_p->setFrameBuffer(m_frameBuffer);
}
}
