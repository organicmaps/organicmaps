#include "map/tiling_render_policy_mt.hpp"

#include "platform/platform.hpp"

#include "graphics/render_context.hpp"

#include "map/window_handle.hpp"
#include "map/tile_renderer.hpp"
#include "map/coverage_generator.hpp"

using namespace graphics;

TilingRenderPolicyMT::TilingRenderPolicyMT(Params const & p)
  : BasicTilingRenderPolicy(p, false)
{
  int cpuCores = GetPlatform().CpuCores();

  graphics::ResourceManager::Params rmp = p.m_rmParams;

  rmp.checkDeviceCaps();
  bool useNpot = rmp.canUseNPOTextures();

  rmp.m_textureParams[ELargeTexture]        = GetTextureParam(GetLargeTextureSize(useNpot), 1, rmp.m_texFormat, ELargeTexture);
  rmp.m_textureParams[EMediumTexture]       = GetTextureParam(GetMediumTextureSize(useNpot), 1, rmp.m_texFormat, EMediumTexture);
  rmp.m_textureParams[ERenderTargetTexture] = GetTextureParam(TileSize(), 1, rmp.m_texRtFormat, ERenderTargetTexture);
  rmp.m_textureParams[ESmallTexture]        = GetTextureParam(GetSmallTextureSize(useNpot), 4, rmp.m_texFormat, ESmallTexture);

  rmp.m_storageParams[ELargeStorage]        = GetStorageParam(50000, 100000, 5, ELargeStorage);
  rmp.m_storageParams[EMediumStorage]       = GetStorageParam(6000, 9000, 1, EMediumStorage);
  rmp.m_storageParams[ESmallStorage]        = GetStorageParam(2000, 4000, 5, ESmallStorage);
  rmp.m_storageParams[ETinyStorage]         = GetStorageParam(100, 200, 5, ETinyStorage);

  rmp.m_glyphCacheParams = GetResourceGlyphCacheParams(Density());

  rmp.m_threadSlotsCount = cpuCores + 2;
  rmp.m_renderThreadsCount = cpuCores;

  rmp.m_useSingleThreadedOGL = false;

  m_resourceManager.reset(new graphics::ResourceManager(rmp, SkinName(), Density()));

  m_primaryRC->setResourceManager(m_resourceManager);
  m_primaryRC->startThreadDrawing(m_resourceManager->guiThreadSlot());

  Platform::FilesList fonts;
  GetPlatform().GetFontNames(fonts);
  m_resourceManager->addFonts(fonts);

  m_drawer.reset(CreateDrawer(p.m_useDefaultFB, p.m_primaryRC, ESmallStorage, ESmallTexture));
  InitCacheScreen();
  InitWindowsHandle(p.m_videoTimer, m_primaryRC);
}

TilingRenderPolicyMT::~TilingRenderPolicyMT()
{
  LOG(LDEBUG, ("cancelling ResourceManager"));
  m_resourceManager->cancel();

  m_CoverageGenerator->Shutdown();
  m_TileRenderer->Shutdown();

  m_CoverageGenerator.reset();
  m_TileRenderer.reset();
}

void TilingRenderPolicyMT::SetRenderFn(TRenderFn const & renderFn)
{
  m_TileRenderer.reset(new TileRenderer(TileSize(),
                                        GetPlatform().CpuCores(),
                                        m_bgColor,
                                        renderFn,
                                        m_primaryRC,
                                        m_resourceManager,
                                        VisualScale(),
                                        0));

  m_CoverageGenerator.reset(new CoverageGenerator(m_TileRenderer.get(),
                                                  m_windowHandle,
                                                  m_primaryRC,
                                                  m_resourceManager,
                                                  0));
}
