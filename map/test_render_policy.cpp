#include "test_render_policy.hpp"
#include "events.hpp"
#include "drawer_yg.hpp"
#include "../yg/base_texture.hpp"
#include "../yg/internal/opengl.hpp"
#include "../yg/utils.hpp"

#include "../geometry/screenbase.hpp"
#include "../platform/platform.hpp"
#include "window_handle.hpp"
#include "../indexer/scales.hpp"

TestRenderPolicy::TestRenderPolicy(VideoTimer * videoTimer,
                                   bool useDefaultFB,
                                   yg::ResourceManager::Params const & rmParams,
                                   shared_ptr<yg::gl::RenderContext> const & primaryRC)
  : RenderPolicy(primaryRC, false, 1)
{
  yg::ResourceManager::Params rmp = rmParams;

  rmp.checkDeviceCaps();

  rmp.m_primaryStoragesParams = yg::ResourceManager::StoragePoolParams(50000 * sizeof(yg::gl::Vertex),
                                                                       sizeof(yg::gl::Vertex),
                                                                       10000 * sizeof(unsigned short),
                                                                       sizeof(unsigned short),
                                                                       15,
                                                                       false,
                                                                       true,
                                                                       1,
                                                                       "primaryStorage",
                                                                       false,
                                                                       false);

  rmp.m_smallStoragesParams = yg::ResourceManager::StoragePoolParams(5000 * sizeof(yg::gl::Vertex),
                                                                     sizeof(yg::gl::Vertex),
                                                                     10000 * sizeof(unsigned short),
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
                                                                    50,
                                                                    true,
                                                                    true,
                                                                    1,
                                                                    "blitStorage",
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
  rmp.fitIntoLimits();

  m_resourceManager.reset(new yg::ResourceManager(rmp));

  Platform::FilesList fonts;
  GetPlatform().GetFontNames(fonts);
  m_resourceManager->addFonts(fonts);

  DrawerYG::params_t p;

  m_primaryFrameBuffer = make_shared_ptr(new yg::gl::FrameBuffer(useDefaultFB));

  p.m_frameBuffer = m_primaryFrameBuffer;
  p.m_resourceManager = m_resourceManager;
  p.m_dynamicPagesCount = 2;
  p.m_textPagesCount = 2;
  p.m_glyphCacheID = m_resourceManager->guiThreadGlyphCacheID();
  p.m_skinName = GetPlatform().SkinName();
  p.m_visualScale = GetPlatform().VisualScale();
  p.m_isSynchronized = true;

  m_drawer.reset(new DrawerYG(p));

  m_windowHandle.reset(new WindowHandle());

  m_windowHandle->setUpdatesEnabled(false);
  m_windowHandle->setVideoTimer(videoTimer);
  m_windowHandle->setRenderPolicy(this);
  m_windowHandle->setRenderContext(primaryRC);

  m_auxFrameBuffer = make_shared_ptr(new yg::gl::FrameBuffer());
  m_frameBuffer = make_shared_ptr(new yg::gl::FrameBuffer());

  m_depthBuffer = make_shared_ptr(new yg::gl::RenderBuffer(512, 512, true));
  m_backBuffer = m_resourceManager->createRenderTarget(512, 512);
  m_actualTarget = m_resourceManager->createRenderTarget(512, 512);

  m_auxFrameBuffer->onSize(512, 512);
  m_frameBuffer->onSize(512, 512);
  m_hasScreen = false;
}

void TestRenderPolicy::DrawFrame(shared_ptr<PaintEvent> const & e,
                                 ScreenBase const & s)
{
  if (!m_hasScreen)
  {
    m_screen = s;
    m_hasScreen = true;
  }

  using namespace yg::gl;

  OGLCHECK(glBindFramebufferFn(GL_FRAMEBUFFER_MWM, m_frameBuffer->id()));
  utils::setupCoordinates(512, 512, false);

  OGLCHECK(glFramebufferTexture2DFn(GL_FRAMEBUFFER_MWM, GL_COLOR_ATTACHMENT0_MWM, GL_TEXTURE_2D, m_backBuffer->id(), 0));
  OGLCHECK(glFramebufferRenderbufferFn(GL_FRAMEBUFFER_MWM, GL_DEPTH_ATTACHMENT_MWM, GL_RENDERBUFFER_MWM, m_depthBuffer->id()));

  make_shared_ptr(new Renderer::ClearCommand(m_bgColor, true, 1.0, true))->perform();

  /// drawing with Z-order

  DrawerYG * pDrawer = e->drawer();

  for (unsigned i = 0; i < 40; ++i)
    pDrawer->screen()->drawRectangle(m2::RectD(10 + i, 10 + i, 110 + i, 110 + i),
                                         yg::Color(255 - (i * 2) % 255, i * 2 % 255, 0, 255),
                                         200 - i);

  pDrawer->screen()->drawRectangle(m2::RectD(80, 80, 180, 180), yg::Color(0, 255, 0, 255), 100);
  pDrawer->screen()->flush(-1);

  /// performing updateActualTarget

  OGLCHECK(glFramebufferTexture2DFn(GL_FRAMEBUFFER_MWM, GL_COLOR_ATTACHMENT0_MWM, GL_TEXTURE_2D, m_actualTarget->id(), 0));

/*  OGLCHECK(glClearColor(m_bgColor.r / 255.0,
                        m_bgColor.g / 255.0,
                        m_bgColor.b / 255.0,
                        m_bgColor.a / 255.0));

  OGLCHECK(glClear(GL_COLOR_BUFFER_BIT));*/

  shared_ptr<Blitter::IMMDrawTexturedRect> immDrawTexturedRect;

  immDrawTexturedRect.reset(
        new Blitter::IMMDrawTexturedRect(m2::RectF(0, 0, m_backBuffer->width(), m_backBuffer->height()),
                                         m2::RectF(0, 0, 1, 1),
                                         m_backBuffer,
                                         m_resourceManager));

  immDrawTexturedRect->perform();
  immDrawTexturedRect.reset();

//  OGLCHECK(glBindFramebufferFn(GL_FRAMEBUFFER_MWM, m_frameBuffer->id()));
  OGLCHECK(glFramebufferTexture2DFn(GL_FRAMEBUFFER_MWM, GL_COLOR_ATTACHMENT0_MWM, GL_TEXTURE_2D, m_backBuffer->id(), 0));

  /// drawing with Z-order

  pDrawer->screen()->drawRectangle(m2::RectD(110, 110, 210, 210), yg::Color(0, 0, 255, 255), 50);
  pDrawer->screen()->drawRectangle(m2::RectD(140, 140, 240, 240), yg::Color(0, 255, 255, 255), 25);
  pDrawer->screen()->flush(-1);

  /// performing last updateActualTarget

//  OGLCHECK(glBindFramebufferFn(GL_FRAMEBUFFER_MWM, m_auxFrameBuffer->id()));
  OGLCHECK(glFramebufferTexture2DFn(GL_FRAMEBUFFER_MWM, GL_COLOR_ATTACHMENT0_MWM, GL_TEXTURE_2D, m_actualTarget->id(), 0));

/*  OGLCHECK(glClearColor(m_bgColor.r / 255.0,
                        m_bgColor.g / 255.0,
                        m_bgColor.b / 255.0,
                        m_bgColor.a / 255.0));

  OGLCHECK(glClear(GL_COLOR_BUFFER_BIT));*/

  immDrawTexturedRect.reset(
        new Blitter::IMMDrawTexturedRect(m2::RectF(0, 0, m_backBuffer->width(), m_backBuffer->height()),
                                         m2::RectF(0, 0, 1, 1),
                                         m_backBuffer,
                                         m_resourceManager));

  immDrawTexturedRect->perform();
  immDrawTexturedRect.reset();

  OGLCHECK(glFramebufferTexture2DFn(GL_FRAMEBUFFER_MWM, GL_COLOR_ATTACHMENT0_MWM, GL_TEXTURE_2D, m_backBuffer->id(), 0));

  pDrawer->screen()->drawRectangle(m2::RectD(90, 150, 190, 250), yg::Color(255, 0, 255, 255), 20);
  pDrawer->screen()->drawRectangle(m2::RectD(120, 180, 220, 280), yg::Color(128, 128, 255, 255), 10);
  pDrawer->screen()->flush(-1);

  /// performing updateActualTarget
  OGLCHECK(glFramebufferTexture2DFn(GL_FRAMEBUFFER_MWM, GL_COLOR_ATTACHMENT0_MWM, GL_TEXTURE_2D, m_actualTarget->id(), 0));

  immDrawTexturedRect.reset(
        new Blitter::IMMDrawTexturedRect(m2::RectF(0, 0, m_backBuffer->width(), m_backBuffer->height()),
                                         m2::RectF(0, 0, 1, 1),
                                         m_backBuffer,
                                         m_resourceManager));

  immDrawTexturedRect->perform();
  immDrawTexturedRect.reset();

  m_primaryFrameBuffer->makeCurrent();
  utils::setupCoordinates(m_primaryFrameBuffer->width(), m_primaryFrameBuffer->height(), true);

  pDrawer->screen()->clear(m_bgColor);

  pDrawer->screen()->blit(m_actualTarget, m_screen, s);
}

m2::RectI const TestRenderPolicy::OnSize(int w, int h)
{
  m_primaryFrameBuffer->onSize(w, h);
  return RenderPolicy::OnSize(w, h);
}
