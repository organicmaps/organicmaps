#include "tstwidgets.hpp"
#include "widgets_impl.hpp"
#include "screen_qt.hpp"

#include "../graphics/screen.hpp"
#include "../graphics/skin.hpp"
#include "../graphics/resource_manager.hpp"

#include "../graphics/opengl/utils.hpp"
#include "../graphics/opengl/framebuffer.hpp"
#include "../graphics/opengl/renderbuffer.hpp"
#include "../graphics/opengl/opengl.hpp"

#include "../platform/platform.hpp"


template class qt::GLDrawWidgetT<graphics::Screen>;

namespace tst {

GLDrawWidget::GLDrawWidget() : base_type(0)
{
}

GLDrawWidget::~GLDrawWidget()
{
  graphics::gl::FinalizeThread();
}

void GLDrawWidget::initializeGL()
{
  try
  {
    graphics::gl::InitExtensions();
    graphics::gl::CheckExtensionSupport();
    graphics::gl::InitializeThread();
  }
  catch (graphics::gl::platform_unsupported & e)
  {
    /// TODO: Show "Please Update Drivers" dialog and close the program.
  }

  m_primaryContext = make_shared_ptr(new qt::gl::RenderContext(this));

  graphics::ResourceManager::Params rmp;

  rmp.m_rtFormat = graphics::Data8Bpp;
  rmp.m_texFormat = graphics::Data8Bpp;

  rmp.m_videoMemoryLimit = 20 * 1024 * 1024;
  rmp.m_primaryStoragesParams = graphics::ResourceManager::StoragePoolParams(30000 * sizeof(graphics::gl::Vertex),
                                                                       sizeof(graphics::gl::Vertex),
                                                                       50000 * sizeof(unsigned short),
                                                                       sizeof(unsigned short),
                                                                       20,
                                                                       false,
                                                                       true,
                                                                       1,
                                                                       "primaryStorage",
                                                                       false,
                                                                       false);

  rmp.m_smallStoragesParams = graphics::ResourceManager::StoragePoolParams(3000 * sizeof(graphics::gl::Vertex),
                                                                     sizeof(graphics::gl::Vertex),
                                                                     5000 * sizeof(unsigned short),
                                                                     sizeof(unsigned short),
                                                                     100,
                                                                     false,
                                                                     true,
                                                                     1,
                                                                     "smallStorage",
                                                                     false,
                                                                     false);

  rmp.m_blitStoragesParams = graphics::ResourceManager::StoragePoolParams(10 * sizeof(graphics::gl::Vertex),
                                                                    sizeof(graphics::gl::Vertex),
                                                                    10 * sizeof(unsigned short),
                                                                    sizeof(unsigned short),
                                                                    30,
                                                                    true,
                                                                    true,
                                                                    1,
                                                                    "blitStorage",
                                                                    false,
                                                                    false);

  rmp.m_multiBlitStoragesParams = graphics::ResourceManager::StoragePoolParams(500 * sizeof(graphics::gl::Vertex),
                                                                         sizeof(graphics::gl::Vertex),
                                                                         500 * sizeof(unsigned short),
                                                                         sizeof(unsigned short),
                                                                         10,
                                                                         true,
                                                                         true,
                                                                         1,
                                                                         "multiBlitStorage",
                                                                         false,
                                                                         false);

  rmp.m_primaryTexturesParams = graphics::ResourceManager::TexturePoolParams(512,
                                                                       512,
                                                                       10,
                                                                       rmp.m_texFormat,
                                                                       true,
                                                                       true,
                                                                       true,
                                                                       1,
                                                                       "primaryTexture",
                                                                       false,
                                                                       false);

  rmp.m_fontTexturesParams = graphics::ResourceManager::TexturePoolParams(512,
                                                                    256,
                                                                    5,
                                                                    rmp.m_texFormat,
                                                                    true,
                                                                    true,
                                                                    true,
                                                                    1,
                                                                    "fontTexture",
                                                                    false,
                                                                    false);

  rmp.m_glyphCacheParams = graphics::ResourceManager::GlyphCacheParams("unicode_blocks.txt",
                                                                 "fonts_whitelist.txt",
                                                                 "fonts_blacklist.txt",
                                                                 2 * 1024 * 1024,
                                                                 1,
                                                                 0);

  rmp.m_useSingleThreadedOGL = false;

  m_resourceManager.reset(new graphics::ResourceManager(rmp));

  Platform::FilesList fonts;
  GetPlatform().GetFontNames(fonts);
  m_resourceManager->addFonts(fonts);

  m_frameBuffer = make_shared_ptr(new graphics::gl::FrameBuffer());

  Drawer::Params params;
  params.m_resourceManager = m_resourceManager;
  params.m_frameBuffer = m_frameBuffer;
  params.m_glyphCacheID = m_resourceManager->guiThreadGlyphCacheID();

  m_p = make_shared_ptr(new graphics::Screen(params));

  m_primaryFrameBuffer = make_shared_ptr(new graphics::gl::FrameBuffer(true));

  m_skin = shared_ptr<graphics::Skin>(loadSkin(m_resourceManager, "basic_mdpi.skn"));
  m_p->setSkin(m_skin);

  params.m_frameBuffer = m_primaryFrameBuffer;
  m_primaryScreen = make_shared_ptr(new graphics::Screen(params));
}

void GLDrawWidget::resizeGL(int w, int h)
{
  m_p->onSize(w, h);
  m_primaryScreen->onSize(w, h);

  m_frameBuffer->onSize(w, h);
  m_primaryFrameBuffer->onSize(w, h);

  m_depthBuffer.reset();
  m_depthBuffer = make_shared_ptr(new graphics::gl::RenderBuffer(w, h, true));
  m_frameBuffer->setDepthBuffer(m_depthBuffer);

  m_renderTarget.reset();
  m_renderTarget = make_shared_ptr(new graphics::gl::RGBA8Texture(w, h));
  m_p->setRenderTarget(m_renderTarget);
}

void GLDrawWidget::paintGL()
{
//  m_renderTarget->dump("renderTarget.png");

  m_p->beginFrame();
  m_p->clear(graphics::Color(182, 182, 182, 255));
  DoDraw(m_p);
  m_p->endFrame();

  m_primaryScreen->beginFrame();

  m_primaryScreen->immDrawTexturedRect(
      m2::RectF(0, 0, m_renderTarget->width(), m_renderTarget->height()),
      m2::RectF(0, 0, 1, 1),
      m_renderTarget
      );

  m_primaryScreen->endFrame();
}
}
