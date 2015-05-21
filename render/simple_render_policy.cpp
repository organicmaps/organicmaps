#include "simple_render_policy.hpp"
#include "events.hpp"
#include "window_handle.hpp"
#include "scales_processor.hpp"

#include "render/drawer.hpp"

#include "graphics/overlay.hpp"
#include "graphics/opengl/opengl.hpp"
#include "graphics/render_context.hpp"

#include "geometry/screenbase.hpp"

#include "platform/platform.hpp"


using namespace graphics;

SimpleRenderPolicy::SimpleRenderPolicy(Params const & p)
  : RenderPolicy(p, 1)
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

  rmp.m_glyphCacheParams = GetResourceGlyphCacheParams(Density());

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

  m_overlay.reset(new graphics::Overlay());
}

void SimpleRenderPolicy::DrawFrame(shared_ptr<PaintEvent> const & e,
                               ScreenBase const & s)
{
#ifndef USE_DRAPE
  shared_ptr<graphics::OverlayStorage> storage(new graphics::OverlayStorage());

  graphics::Screen * pScreen = GPUDrawer::GetScreen(e->drawer());

  pScreen->setOverlay(storage);
  pScreen->beginFrame();
  pScreen->clear(m_bgColor);

  m_renderFn(e, s, s.PixelRect(), ScalesProcessor().GetTileScaleBase(s));

  pScreen->resetOverlay();
  pScreen->clear(graphics::Color::White(), false, 1.0, true);

  m_overlay->merge(storage);

  math::Matrix<double, 3, 3> const m = math::Identity<double, 3>();
  m_overlay->forEach([&pScreen, &m](shared_ptr<graphics::OverlayElement> const & e)
  {
    e->draw(pScreen, m);
  });

  pScreen->endFrame();
#endif // USE_DRAPE
}

graphics::Overlay * SimpleRenderPolicy::FrameOverlay() const
{
  return m_overlay.get();
}
