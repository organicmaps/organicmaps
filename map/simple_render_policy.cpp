#include "simple_render_policy.hpp"
#include "events.hpp"
#include "drawer.hpp"
#include "window_handle.hpp"

#include "../graphics/overlay.hpp"
#include "../graphics/opengl/opengl.hpp"
#include "../graphics/render_context.hpp"

#include "../indexer/scales.hpp"
#include "../geometry/screenbase.hpp"

#include "../platform/platform.hpp"

SimpleRenderPolicy::SimpleRenderPolicy(Params const & p)
  : RenderPolicy(p, false, 1)
{
  graphics::ResourceManager::Params rmp = p.m_rmParams;

  rmp.checkDeviceCaps();

  graphics::ResourceManager::TexturePoolParams tpp;
  graphics::ResourceManager::StoragePoolParams spp;

  spp = graphics::ResourceManager::StoragePoolParams(50000 * sizeof(graphics::gl::Vertex),
                                                     sizeof(graphics::gl::Vertex),
                                                     10000 * sizeof(unsigned short),
                                                     sizeof(unsigned short),
                                                     15,
                                                     graphics::ELargeStorage,
                                                     false);

  rmp.m_storageParams[spp.m_storageType] = spp;

  spp = graphics::ResourceManager::StoragePoolParams(5000 * sizeof(graphics::gl::Vertex),
                                                     sizeof(graphics::gl::Vertex),
                                                     10000 * sizeof(unsigned short),
                                                     sizeof(unsigned short),
                                                     100,
                                                     graphics::EMediumStorage,
                                                     false);

  rmp.m_storageParams[spp.m_storageType] = spp;

  spp = graphics::ResourceManager::StoragePoolParams(2000 * sizeof(graphics::gl::Vertex),
                                                     sizeof(graphics::gl::Vertex),
                                                     6000 * sizeof(unsigned short),
                                                     sizeof(unsigned short),
                                                     10,
                                                     graphics::ESmallStorage,
                                                     false);

  rmp.m_storageParams[spp.m_storageType] = spp;


  tpp = graphics::ResourceManager::TexturePoolParams(512,
                                                     256,
                                                     10,
                                                     rmp.m_texFormat,
                                                     graphics::ELargeTexture,
                                                     false);

  rmp.m_textureParams[tpp.m_textureType] = tpp;

  tpp = graphics::ResourceManager::TexturePoolParams(512,
                                                     256,
                                                     5,
                                                     rmp.m_texFormat,
                                                     graphics::EMediumTexture,
                                                     false);

  rmp.m_textureParams[tpp.m_textureType] = tpp;

  rmp.m_glyphCacheParams = graphics::ResourceManager::GlyphCacheParams("unicode_blocks.txt",
                                                                 "fonts_whitelist.txt",
                                                                 "fonts_blacklist.txt",
                                                                 2 * 1024 * 1024,
                                                                 Density());

  rmp.m_renderThreadsCount = 0;
  rmp.m_threadSlotsCount = 1;

  rmp.m_useSingleThreadedOGL = false;

  m_resourceManager.reset(new graphics::ResourceManager(rmp, SkinName(), Density()));

  m_primaryRC->setResourceManager(m_resourceManager);
  m_primaryRC->startThreadDrawing(m_resourceManager->guiThreadSlot());

  Platform::FilesList fonts;
  GetPlatform().GetFontNames(fonts);
  m_resourceManager->addFonts(fonts);

  Drawer::Params dp;

  dp.m_frameBuffer = make_shared_ptr(new graphics::gl::FrameBuffer(p.m_useDefaultFB));
  dp.m_resourceManager = m_resourceManager;
  dp.m_threadSlot = m_resourceManager->guiThreadSlot();
  dp.m_visualScale = VisualScale();
  dp.m_isSynchronized = true;

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
  size_t const scaleEtalonSize = 512;

  m2::RectD glbRect;
  m2::PointD const pxCenter = s.PixelRect().Center();
  s.PtoG(m2::RectD(pxCenter - m2::PointD(scaleEtalonSize / 2, scaleEtalonSize / 2),
                   pxCenter + m2::PointD(scaleEtalonSize / 2, scaleEtalonSize / 2)),
         glbRect);

  shared_ptr<graphics::Overlay> overlay(new graphics::Overlay());

  Drawer * pDrawer = e->drawer();
  graphics::Screen * pScreen = pDrawer->screen();

  pScreen->setOverlay(overlay);
  pScreen->beginFrame();
  pScreen->clear(m_bgColor);

  /// @todo Need to review this policy (using in Map server).
  m_renderFn(e, s, s.PixelRect(), scales::GetScaleLevel(glbRect));

  overlay->draw(pScreen, math::Identity<double, 3>());
  pScreen->resetOverlay();

  pScreen->endFrame();
}
