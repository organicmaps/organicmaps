#include "yopme_render_policy.hpp"
#include "window_handle.hpp"

#include "../base/matrix.hpp"
#include "../indexer/scales.hpp"
#include "../geometry/screenbase.hpp"

#include "../graphics/opengl/framebuffer.hpp"
#include "../graphics/render_context.hpp"
#include "../graphics/blitter.hpp"
#include "../graphics/display_list.hpp"
#include "../platform/platform.hpp"

#include "../std/vector.hpp"

YopmeRP::YopmeRP(RenderPolicy::Params const & p)
  : RenderPolicy(p, false, 1)
{
  LOG(LDEBUG, ("Yopme render policy created"));
  graphics::ResourceManager::Params rmp = p.m_rmParams;

  rmp.checkDeviceCaps();

  graphics::ResourceManager::TexturePoolParams tpp;
  graphics::ResourceManager::StoragePoolParams spp;

  double k = VisualScale();

  spp = graphics::ResourceManager::StoragePoolParams(50000 * sizeof(graphics::gl::Vertex),
                                                     sizeof(graphics::gl::Vertex),
                                                     10000 * sizeof(unsigned short),
                                                     sizeof(unsigned short),
                                                     15,
                                                     graphics::ELargeStorage,
                                                     false);

  rmp.m_storageParams[spp.m_storageType] = spp;

  spp = graphics::ResourceManager::StoragePoolParams(2000 * sizeof(graphics::gl::Vertex),
                                                     sizeof(graphics::gl::Vertex),
                                                     6000 * sizeof(unsigned short),
                                                     sizeof(unsigned short),
                                                     1,
                                                     graphics::ESmallStorage,
                                                     false);

  rmp.m_storageParams[spp.m_storageType] = spp;

  tpp = graphics::ResourceManager::TexturePoolParams(512 * k,
                                                     256 * k,
                                                     1,
                                                     rmp.m_texFormat,
                                                     graphics::ELargeTexture,
                                                     false);

  rmp.m_textureParams[tpp.m_textureType] = tpp;

  rmp.m_glyphCacheParams = graphics::ResourceManager::GlyphCacheParams("unicode_blocks.txt",
                                                                 "fonts_whitelist.txt",
                                                                 "fonts_blacklist.txt",
                                                                 2 * 1024 * 1024,
                                                                 Density());

  rmp.m_renderThreadsCount = 0;
  rmp.m_threadSlotsCount = 1;
  rmp.m_useSingleThreadedOGL = true;

  m_resourceManager.reset(new graphics::ResourceManager(rmp, SkinName(), Density()));

  m_primaryRC->setResourceManager(m_resourceManager);
  m_primaryRC->startThreadDrawing(m_resourceManager->guiThreadSlot());

  Platform::FilesList fonts;
  GetPlatform().GetFontNames(fonts);
  m_resourceManager->addFonts(fonts);

  // init main drawer.
  Drawer::Params dp;
  dp.m_frameBuffer = make_shared_ptr(new graphics::gl::FrameBuffer(p.m_useDefaultFB));
  dp.m_resourceManager = m_resourceManager;
  dp.m_threadSlot = m_resourceManager->guiThreadSlot();
  dp.m_visualScale = VisualScale();
  dp.m_isSynchronized = true;
  dp.m_renderContext = m_primaryRC;
  m_drawer.reset(new Drawer(dp));
  m_drawer->onSize(p.m_screenWidth, p.m_screenHeight);

  // init offscreen drawer
  dp.m_frameBuffer = make_shared_ptr(new graphics::gl::FrameBuffer());
  dp.m_isSynchronized = false;
  dp.m_doUnbindRT = false;
  m_pDrawer.reset(new Drawer(dp));
  m_pDrawer->onSize(p.m_screenWidth, p.m_screenHeight);
  m_pDrawer->screen()->setDepthBuffer(make_shared_ptr(new graphics::gl::RenderBuffer(p.m_screenWidth, p.m_screenHeight, true)));

  m_windowHandle.reset(new WindowHandle());
  m_windowHandle->setUpdatesEnabled(false);
  m_windowHandle->setRenderPolicy(this);
  m_windowHandle->setVideoTimer(p.m_videoTimer);
  m_windowHandle->setRenderContext(p.m_primaryRC);
}

void YopmeRP::DrawFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s)
{
  shared_ptr<graphics::gl::BaseTexture> renderTarget;
  ASSERT(m_pDrawer->screen()->width() == GetDrawer()->screen()->width(), ());
  ASSERT(m_pDrawer->screen()->height() == GetDrawer()->screen()->height(), ());

  int width = 0;
  int height = 0;
  shared_ptr<graphics::Overlay> overlay(new graphics::Overlay());
  overlay->setCouldOverlap(true);

  { // offscreen rendering
    graphics::Screen * pScreen = m_pDrawer->screen();
    width = pScreen->width();
    height = pScreen->height();

    m2::RectI renderRect(0, 0, width, height);
    pScreen->setOverlay(overlay);

    renderTarget = m_resourceManager->createRenderTarget(width, height);
    pScreen->setRenderTarget(renderTarget);
    pScreen->beginFrame();
    pScreen->setClipRect(renderRect);
    pScreen->clear(m_bgColor);

    shared_ptr<PaintEvent> paintEvent(new PaintEvent(m_pDrawer.get()));
    m_renderFn(paintEvent, s, m2::RectD(renderRect), scales::GetScaleLevel(s.GlobalRect().GetGlobalRect()));

    pScreen->endFrame();
    pScreen->resetOverlay();

    pScreen->unbindRenderTarget();
  }

  overlay->clip(m2::RectI(0, 0, width, height));
  shared_ptr<graphics::Overlay> drawOverlay(new graphics::Overlay());
  drawOverlay->setCouldOverlap(false);
  drawOverlay->merge(*overlay);

  {
    // on screen rendering
    graphics::Screen * pScreen = GetDrawer()->screen();
    graphics::BlitInfo info;
    info.m_srcSurface = renderTarget;
    info.m_srcRect = m2::RectI(0, 0, width, height);
    info.m_texRect = m2::RectU(1, 1, width - 1, height - 1);
    info.m_matrix = math::Identity<double, 3>();

    pScreen->beginFrame();
    pScreen->clear(m_bgColor);

    pScreen->applyBlitStates();
    pScreen->blit(&info, 1, true, graphics::maxDepth);

    pScreen->applyStates();
    drawOverlay->draw(pScreen, math::Identity<double, 3>());

    pScreen->endFrame();
  }
}

void YopmeRP::OnSize(int w, int h)
{
  RenderPolicy::OnSize(w, h);
  m_pDrawer->onSize(w, h);
  m_pDrawer->screen()->setDepthBuffer(make_shared_ptr(new graphics::gl::RenderBuffer(w, h, true)));
}
