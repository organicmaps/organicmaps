#include "tstwidgets.hpp"
#include "widgets_impl.hpp"
#include "screen_qt.hpp"

#include "../yg/screen.hpp"
#include "../yg/utils.hpp"
#include "../yg/skin.hpp"
#include "../yg/framebuffer.hpp"
#include "../yg/renderbuffer.hpp"
#include "../yg/resource_manager.hpp"

#ifdef OMIM_OS_WINDOWS
#include "../yg/internal/opengl_win32.hpp"
#endif

#include "../platform/platform.hpp"

#include "../base/start_mem_debug.hpp"


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
#ifdef OMIM_OS_WINDOWS
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
      10 * sizeof(yg::gl::AuxVertex),
      10 * sizeof(unsigned short),
      30,
      512, 256, 15,
      GetPlatform().ReadPathForFile("unicode_blocks.txt").c_str(),
      GetPlatform().ReadPathForFile("fonts_whitelist.txt").c_str(),
      GetPlatform().ReadPathForFile("fonts_blacklist.txt").c_str(),
      2000000));

  m_resourceManager->addFonts(GetPlatform().GetFontNames());

  m_frameBuffer = make_shared_ptr(new yg::gl::FrameBuffer());

  DrawerYG::Params params;
  params.m_resourceManager = m_resourceManager;
  params.m_isMultiSampled = false;
  params.m_frameBuffer = m_frameBuffer;

  m_p = make_shared_ptr(new yg::gl::Screen(params));

  m_primaryFrameBuffer = make_shared_ptr(new yg::gl::FrameBuffer(true));

  m_skin = shared_ptr<yg::Skin>(loadSkin(m_resourceManager, GetPlatform().SkinName(), params.m_dynamicPagesCount, params.m_textPagesCount));
  m_p->setSkin(m_skin);

  params.m_frameBuffer = m_primaryFrameBuffer;
  m_primaryScreen = make_shared_ptr(new yg::gl::Screen(params));
}

void GLDrawWidget::resizeGL(int w, int h)
{
  m_p->onSize(w, h);
  m_primaryScreen->onSize(w, h);

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

//  m_renderTarget->dump("renderTarget.png");

  m_primaryScreen->beginFrame();

  m_primaryScreen->immDrawTexturedRect(
      m2::RectF(0, 0, m_renderTarget->width(), m_renderTarget->height()),
      m2::RectF(0, 0, 1, 1),
      m_renderTarget
      );

  m_primaryScreen->endFrame();
}
}
