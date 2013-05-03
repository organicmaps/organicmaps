#include "tiling_render_policy_mt.hpp"

#include "../platform/platform.hpp"

#include "../graphics/render_context.hpp"

#include "window_handle.hpp"
#include "tile_renderer.hpp"
#include "coverage_generator.hpp"

using namespace graphics;

TilingRenderPolicyMT::TilingRenderPolicyMT(Params const & p)
  : BasicTilingRenderPolicy(p,
                            false)
{
  int cpuCores = GetPlatform().CpuCores();

  graphics::ResourceManager::Params rmp = p.m_rmParams;

  rmp.checkDeviceCaps();

  ResourceManager::TexturePoolParams tpp;
  ResourceManager::StoragePoolParams spp;

  int k = int(ceil(VisualScale()));

  tpp = ResourceManager::TexturePoolParams(512,
                                           512,
                                           1,
                                           rmp.m_texFormat,
                                           ELargeTexture,
                                           true);

  rmp.m_textureParams[tpp.m_textureType] = tpp;

  tpp = ResourceManager::TexturePoolParams(256 * k,
                                           256 * k,
                                           1,
                                           rmp.m_texFormat,
                                           EMediumTexture,
                                           true);

  rmp.m_textureParams[tpp.m_textureType] = tpp;

  tpp = ResourceManager::TexturePoolParams(TileSize(),
                                           TileSize(),
                                           1,
                                           rmp.m_texRtFormat,
                                           ERenderTargetTexture,
                                           true);

  rmp.m_textureParams[tpp.m_textureType] = tpp;

  tpp = ResourceManager::TexturePoolParams(128 * k,
                                           128 * k,
                                           4,
                                           rmp.m_texFormat,
                                           ESmallTexture,
                                           true);

  rmp.m_textureParams[tpp.m_textureType] = tpp;

  spp = ResourceManager::StoragePoolParams(50000 * sizeof(graphics::gl::Vertex),
                                           sizeof(graphics::gl::Vertex),
                                           100000 * sizeof(unsigned short),
                                           sizeof(unsigned short),
                                           5,
                                           ELargeStorage,
                                           true);

  rmp.m_storageParams[spp.m_storageType] = spp;

  spp = ResourceManager::StoragePoolParams(6000 * sizeof(graphics::gl::Vertex),
                                           sizeof(graphics::gl::Vertex),
                                           9000 * sizeof(unsigned short),
                                           sizeof(unsigned short),
                                           1,
                                           EMediumStorage,
                                           true);

  rmp.m_storageParams[spp.m_storageType] = spp;

  spp = ResourceManager::StoragePoolParams(2000 * sizeof(graphics::gl::Vertex),
                                           sizeof(graphics::gl::Vertex),
                                           4000 * sizeof(unsigned short),
                                           sizeof(unsigned short),
                                           5,
                                           ESmallStorage,
                                           true);

  rmp.m_storageParams[spp.m_storageType] = spp;

  spp = ResourceManager::StoragePoolParams(100 * sizeof(graphics::gl::Vertex),
                                           sizeof(graphics::gl::Vertex),
                                           200 * sizeof(unsigned short),
                                           sizeof(unsigned short),
                                           1,
                                           ETinyStorage,
                                           true);

  rmp.m_storageParams[spp.m_storageType] = spp;


  rmp.m_glyphCacheParams = graphics::ResourceManager::GlyphCacheParams("unicode_blocks.txt",
                                                                       "fonts_whitelist.txt",
                                                                       "fonts_blacklist.txt",
                                                                       2 * 1024 * 1024,
                                                                       Density());

  rmp.m_threadSlotsCount = cpuCores + 2;
  rmp.m_renderThreadsCount = cpuCores;

  rmp.m_useSingleThreadedOGL = false;

  m_resourceManager.reset(new graphics::ResourceManager(rmp));

  m_primaryRC->setResourceManager(m_resourceManager);
  m_primaryRC->startThreadDrawing(m_resourceManager->guiThreadSlot());

  Platform::FilesList fonts;
  GetPlatform().GetFontNames(fonts);
  m_resourceManager->addFonts(fonts);

  Drawer::Params dp;

  dp.m_frameBuffer = make_shared_ptr(new graphics::gl::FrameBuffer(p.m_useDefaultFB));
  dp.m_resourceManager = m_resourceManager;
  dp.m_threadSlot = m_resourceManager->guiThreadSlot();
  dp.m_skinName = SkinName();
  dp.m_visualScale = VisualScale();
  dp.m_storageType = ESmallStorage;
  dp.m_textureType = ESmallTexture;
  dp.m_isSynchronized = false;
  dp.m_fastSolidPath = true;
  dp.m_renderContext = p.m_primaryRC;
  dp.m_density = Density();

  m_drawer.reset(new Drawer(dp));

  InitCacheScreen();

  m_windowHandle.reset(new WindowHandle());

  m_windowHandle->setUpdatesEnabled(false);
  m_windowHandle->setRenderPolicy(this);
  m_windowHandle->setVideoTimer(p.m_videoTimer);
  m_windowHandle->setRenderContext(p.m_primaryRC);
}

TilingRenderPolicyMT::~TilingRenderPolicyMT()
{
  LOG(LINFO, ("cancelling ResourceManager"));
  m_resourceManager->cancel();

  m_CoverageGenerator.reset();

  m_TileRenderer->ClearCommands();
  m_TileRenderer->SetSequenceID(numeric_limits<int>::max());
  m_TileRenderer->CancelCommands();
  m_TileRenderer->WaitForEmptyAndFinished();

  m_TileRenderer.reset();
}

void TilingRenderPolicyMT::SetRenderFn(TRenderFn renderFn)
{
  string skinName = SkinName();

  m_TileRenderer.reset(new TileRenderer(TileSize(),
                                        skinName,
                                        Density(),
                                        GetPlatform().CpuCores(),
                                        m_bgColor,
                                        renderFn,
                                        m_primaryRC,
                                        m_resourceManager,
                                        VisualScale(),
                                        0));

  m_CoverageGenerator.reset(new CoverageGenerator(skinName,
                                                  Density(),
                                                  m_TileRenderer.get(),
                                                  m_windowHandle,
                                                  m_primaryRC,
                                                  m_resourceManager,
                                                  0,
                                                  m_countryIndexFn));
}
