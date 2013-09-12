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

using namespace graphics;

SimpleRenderPolicy::SimpleRenderPolicy(Params const & p)
  : RenderPolicy(p, false, 1)
{
  graphics::ResourceManager::Params rmp = p.m_rmParams;

  rmp.checkDeviceCaps();
  bool useNpot = rmp.canUseNPOTextures();

  rmp.m_textureParams[ELargeTexture]        = GetTextureParam(GetLargeTextureSize(useNpot), 10, rmp.m_texFormat, ELargeTexture);
  rmp.m_textureParams[EMediumTexture]       = GetTextureParam(GetMediumTextureSize(useNpot), 5, rmp.m_texFormat, EMediumTexture);
  rmp.m_textureParams[ESmallTexture]        = GetTextureParam(GetSmallTextureSize(useNpot), 4, rmp.m_texFormat, ESmallTexture);

  rmp.m_storageParams[ELargeStorage]        = GetStorageParam(50000, 100000, 15, ELargeStorage);
  rmp.m_storageParams[EMediumStorage]       = GetStorageParam(5000, 10000, 100, EMediumStorage);
  rmp.m_storageParams[ESmallStorage]        = GetStorageParam(2000, 6000, 10, ESmallStorage);
  rmp.m_storageParams[ETinyStorage]         = GetStorageParam(100, 200, 1, ETinyStorage);

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

  m_drawer.reset(CreateDrawer(p.m_useDefaultFB, p.m_primaryRC, ELargeStorage, ELargeTexture));
  InitCacheScreen();
  InitWindowsHandle(p.m_videoTimer, p.m_primaryRC);
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
  overlay->setCouldOverlap(false);

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
