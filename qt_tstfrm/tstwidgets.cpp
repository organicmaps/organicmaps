#include "tstwidgets.hpp"
#include "widgets_impl.hpp"
#include "screen_qt.hpp"

#include "../yg/screen.hpp"
#include "../yg/utils.hpp"
#include "../yg/skin.hpp"
#include "../yg/framebuffer.hpp"
#include "../yg/renderbuffer.hpp"
#include "../yg/resource_manager.hpp"
#include "../yg/internal/opengl.hpp"

#include "../platform/platform.hpp"


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
  try
  {
    yg::gl::InitExtensions();
    yg::gl::CheckExtensionSupport();
  }
  catch (yg::gl::platform_unsupported & e)
  {
    /// TODO: Show "Please Update Drivers" dialog and close the program.
  }

  m_primaryContext = make_shared_ptr(new qt::gl::RenderContext(this));

  yg::ResourceManager::Params rmp;

  rmp.m_videoMemoryLimit = 20 * 1024 * 1024;
  rmp.m_primaryStoragesParams = yg::ResourceManager::StoragePoolParams(30000 * sizeof(yg::gl::Vertex),
                                                                       sizeof(yg::gl::Vertex),
                                                                       50000 * sizeof(unsigned short),
                                                                       sizeof(unsigned short),
                                                                       20,
                                                                       false,
                                                                       true,
                                                                       1,
                                                                       "primaryStorage",
                                                                       false,
                                                                       false);

  rmp.m_smallStoragesParams = yg::ResourceManager::StoragePoolParams(3000 * sizeof(yg::gl::Vertex),
                                                                     sizeof(yg::gl::Vertex),
                                                                     5000 * sizeof(unsigned short),
                                                                     sizeof(unsigned short),
                                                                     100,
                                                                     false,
                                                                     true,
                                                                     1,
                                                                     "smallStorage",
                                                                     false,
                                                                     false);

  rmp.m_blitStoragesParams = yg::ResourceManager::StoragePoolParams(10 * sizeof(yg::gl::Vertex),
                                                                    sizeof(yg::gl::Vertex),
                                                                    10 * sizeof(unsigned short),
                                                                    sizeof(unsigned short),
                                                                    30,
                                                                    true,
                                                                    true,
                                                                    1,
                                                                    "blitStorage",
                                                                    false,
                                                                    false);

  rmp.m_multiBlitStoragesParams = yg::ResourceManager::StoragePoolParams(500 * sizeof(yg::gl::Vertex),
                                                                         sizeof(yg::gl::Vertex),
                                                                         500 * sizeof(unsigned short),
                                                                         sizeof(unsigned short),
                                                                         10,
                                                                         true,
                                                                         true,
                                                                         1,
                                                                         "multiBlitStorage",
                                                                         false,
                                                                         false);

  rmp.m_primaryTexturesParams = yg::ResourceManager::TexturePoolParams(512,
                                                                       256,
                                                                       10,
                                                                       rmp.m_texFormat,
                                                                       true,
                                                                       true,
                                                                       true,
                                                                       1,
                                                                       "primaryTexture",
                                                                       false,
                                                                       false);

  rmp.m_fontTexturesParams = yg::ResourceManager::TexturePoolParams(512,
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

  rmp.m_glyphCacheParams = yg::ResourceManager::GlyphCacheParams("unicode_blocks.txt",
                                                                 "fonts_whitelist.txt",
                                                                 "fonts_blacklist.txt",
                                                                 2 * 1024 * 1024,
                                                                 1,
                                                                 0);

  rmp.m_useSingleThreadedOGL = false;
  rmp.m_useVA = !yg::gl::g_isBufferObjectsSupported;
  rmp.m_rtFormat = yg::Data8Bpp;
  rmp.m_texFormat = yg::Data8Bpp;

  m_resourceManager.reset(new yg::ResourceManager(rmp));

  Platform::FilesList fonts;
  GetPlatform().GetFontNames(fonts);
  m_resourceManager->addFonts(fonts);

  m_frameBuffer = make_shared_ptr(new yg::gl::FrameBuffer());

  DrawerYG::Params params;
  params.m_resourceManager = m_resourceManager;
  params.m_frameBuffer = m_frameBuffer;
  params.m_glyphCacheID = m_resourceManager->guiThreadGlyphCacheID();

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
