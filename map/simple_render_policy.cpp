#include "simple_render_policy.hpp"
#include "events.hpp"
#include "drawer.hpp"
#include "window_handle.hpp"

#include "../graphics/overlay.hpp"
#include "../graphics/opengl/opengl.hpp"
#include "../graphics/skin.hpp"

#include "../indexer/scales.hpp"
#include "../geometry/screenbase.hpp"

#include "../platform/platform.hpp"

SimpleRenderPolicy::SimpleRenderPolicy(Params const & p)
  : RenderPolicy(p, false, 1)
{
  graphics::ResourceManager::Params rmp = p.m_rmParams;

  rmp.checkDeviceCaps();

  rmp.m_primaryStoragesParams = graphics::ResourceManager::StoragePoolParams(50000 * sizeof(graphics::gl::Vertex),
                                                                       sizeof(graphics::gl::Vertex),
                                                                       10000 * sizeof(unsigned short),
                                                                       sizeof(unsigned short),
                                                                       15,
                                                                       false,
                                                                       true,
                                                                       1,
                                                                       "primaryStorage",
                                                                       false,
                                                                       false);

  rmp.m_smallStoragesParams = graphics::ResourceManager::StoragePoolParams(5000 * sizeof(graphics::gl::Vertex),
                                                                     sizeof(graphics::gl::Vertex),
                                                                     10000 * sizeof(unsigned short),
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
                                                                    50,
                                                                    true,
                                                                    true,
                                                                    1,
                                                                    "blitStorage",
                                                                    false,
                                                                    false);

  rmp.m_primaryTexturesParams = graphics::ResourceManager::TexturePoolParams(512,
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
  rmp.fitIntoLimits();

  m_resourceManager.reset(new graphics::ResourceManager(rmp));

  Platform::FilesList fonts;
  GetPlatform().GetFontNames(fonts);
  m_resourceManager->addFonts(fonts);

  Drawer::Params dp;

  dp.m_frameBuffer = make_shared_ptr(new graphics::gl::FrameBuffer(p.m_useDefaultFB));
  dp.m_resourceManager = m_resourceManager;
  dp.m_glyphCacheID = m_resourceManager->guiThreadGlyphCacheID();
  dp.m_skin = make_shared_ptr(graphics::loadSkin(m_resourceManager, SkinName()));
  dp.m_visualScale = VisualScale();
  dp.m_isSynchronized = true;
  dp.m_fastSolidPath = false;

  m_drawer.reset(new Drawer(dp));

  m_windowHandle.reset(new WindowHandle());

  m_windowHandle->setUpdatesEnabled(false);
  m_windowHandle->setRenderPolicy(this);
  m_windowHandle->setVideoTimer(p.m_videoTimer);
  m_windowHandle->setRenderContext(p.m_primaryRC);
}

void SimpleRenderPolicy::DrawFrame(shared_ptr<PaintEvent> const & e,
                               ScreenBase const & s)
{
  int scaleEtalonSize = GetPlatform().ScaleEtalonSize();

  m2::RectD glbRect;
  m2::PointD const pxCenter = s.PixelRect().Center();
  s.PtoG(m2::RectD(pxCenter - m2::PointD(scaleEtalonSize / 2, scaleEtalonSize / 2),
                   pxCenter + m2::PointD(scaleEtalonSize / 2, scaleEtalonSize / 2)),
         glbRect);

  shared_ptr<graphics::Overlay> overlay(new graphics::Overlay());

  Drawer * pDrawer = e->drawer();

  pDrawer->screen()->setOverlay(overlay);
  pDrawer->screen()->beginFrame();
  pDrawer->screen()->clear(m_bgColor);

  m_renderFn(e, s, s.ClipRect(), s.ClipRect(), scales::GetScaleLevel(glbRect), false);

  overlay->draw(pDrawer->screen().get(), math::Identity<double, 3>());
  pDrawer->screen()->resetOverlay();

  pDrawer->screen()->endFrame();
}
