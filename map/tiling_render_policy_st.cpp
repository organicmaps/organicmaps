#include "tiling_render_policy_st.hpp"

#include "../platform/platform.hpp"

#include "../graphics/opengl/opengl.hpp"
#include "../graphics/render_context.hpp"

#include "window_handle.hpp"
#include "queued_renderer.hpp"
#include "tile_renderer.hpp"
#include "coverage_generator.hpp"

TilingRenderPolicyST::TilingRenderPolicyST(Params const & p)
  : BasicTilingRenderPolicy(p,
                            true)
{
  int cpuCores = GetPlatform().CpuCores();

  graphics::ResourceManager::Params rmp = p.m_rmParams;

  rmp.checkDeviceCaps();

  graphics::ResourceManager::TexturePoolParams tpp;
  graphics::ResourceManager::StoragePoolParams spp;

  int k = int(ceil(VisualScale()));

  tpp = graphics::ResourceManager::TexturePoolParams(512,
                                                     512,
                                                     1,
                                                     rmp.m_texFormat,
                                                     graphics::ELargeTexture,
                                                     true);

  rmp.m_textureParams[tpp.m_textureType] = tpp;

  tpp = graphics::ResourceManager::TexturePoolParams(256 * k,
                                                     256 * k,
                                                     10,
                                                     rmp.m_texFormat,
                                                     graphics::EMediumTexture,
                                                     true);

  rmp.m_textureParams[tpp.m_textureType] = tpp;

  tpp = graphics::ResourceManager::TexturePoolParams(TileSize(),
                                                     TileSize(),
                                                     1,
                                                     rmp.m_texRtFormat,
                                                     graphics::ERenderTargetTexture,
                                                     true);

  rmp.m_textureParams[tpp.m_textureType] = tpp;

  tpp = graphics::ResourceManager::TexturePoolParams(128 * k,
                                                     128 * k,
                                                     2,
                                                     rmp.m_texFormat,
                                                     graphics::ESmallTexture,
                                                     true);

  rmp.m_textureParams[tpp.m_textureType] = tpp;

  spp = graphics::ResourceManager::StoragePoolParams(6000 * sizeof(graphics::gl::Vertex),
                                                     sizeof(graphics::gl::Vertex),
                                                     9000 * sizeof(unsigned short),
                                                     sizeof(unsigned short),
                                                     10,
                                                     graphics::ELargeStorage,
                                                     true);

  rmp.m_storageParams[spp.m_storageType] = spp;

  spp = graphics::ResourceManager::StoragePoolParams(6000 * sizeof(graphics::gl::Vertex),
                                                     sizeof(graphics::gl::Vertex),
                                                     9000 * sizeof(unsigned short),
                                                     sizeof(unsigned short),
                                                     1,
                                                     graphics::EMediumStorage,
                                                     true);

  rmp.m_storageParams[spp.m_storageType] = spp;

  spp = graphics::ResourceManager::StoragePoolParams(2000 * sizeof(graphics::gl::Vertex),
                                                     sizeof(graphics::gl::Vertex),
                                                     4000 * sizeof(unsigned short),
                                                     sizeof(unsigned short),
                                                     5,
                                                     graphics::ESmallStorage,
                                                     true);

  rmp.m_storageParams[spp.m_storageType] = spp;

  spp = graphics::ResourceManager::StoragePoolParams(100 * sizeof(graphics::gl::Vertex),
                                                     sizeof(graphics::gl::Vertex),
                                                     200 * sizeof(unsigned short),
                                                     sizeof(unsigned short),
                                                     5,
                                                     graphics::ETinyStorage,
                                                     true);

  rmp.m_storageParams[spp.m_storageType] = spp;

  rmp.m_glyphCacheParams = graphics::ResourceManager::GlyphCacheParams("unicode_blocks.txt",
                                                                       "fonts_whitelist.txt",
                                                                       "fonts_blacklist.txt",
                                                                       2 * 1024 * 1024,
                                                                       Density());

  rmp.m_threadSlotsCount = cpuCores + 2;
  rmp.m_renderThreadsCount = cpuCores;

  rmp.m_useSingleThreadedOGL = true;

  m_resourceManager.reset(new graphics::ResourceManager(rmp));

  m_primaryRC->setResourceManager(m_resourceManager);
  m_primaryRC->startThreadDrawing(m_resourceManager->guiThreadSlot());

  m_QueuedRenderer->SetSinglePipelineProcessing(m_resourceManager->useReadPixelsToSynchronize());

  Platform::FilesList fonts;
  GetPlatform().GetFontNames(fonts);
  m_resourceManager->addFonts(fonts);

  Drawer::Params dp;

  dp.m_frameBuffer = make_shared_ptr(new graphics::gl::FrameBuffer(p.m_useDefaultFB));
  dp.m_resourceManager = m_resourceManager;
  dp.m_threadSlot = m_resourceManager->guiThreadSlot();
  dp.m_skinName = SkinName();
  dp.m_visualScale = VisualScale();
  dp.m_storageType = graphics::ESmallStorage;
  dp.m_textureType = graphics::ESmallTexture;
  dp.m_isSynchronized = false;
  dp.m_fastSolidPath = true;
  dp.m_renderContext = p.m_primaryRC;
  dp.m_density = Density();

//  p.m_isDebugging = true;

  m_drawer.reset(new Drawer(dp));

  InitCacheScreen();

  m_windowHandle.reset(new WindowHandle());

  m_windowHandle->setUpdatesEnabled(false);
  m_windowHandle->setRenderPolicy(this);
  m_windowHandle->setVideoTimer(p.m_videoTimer);
  m_windowHandle->setRenderContext(p.m_primaryRC);
}

TilingRenderPolicyST::~TilingRenderPolicyST()
{
  LOG(LINFO, ("cancelling ResourceManager"));
  m_resourceManager->cancel();

  int cpuCores = GetPlatform().CpuCores();

  LOG(LINFO, ("deleting TilingRenderPolicyST"));

  m_QueuedRenderer->PrepareQueueCancellation(cpuCores);

  /// now we should process all commands to collect them into queues
  m_CoverageGenerator->SetSequenceID(numeric_limits<int>::max());
  m_CoverageGenerator->WaitForEmptyAndFinished();

  m_QueuedRenderer->CancelQueuedCommands(cpuCores);

  LOG(LINFO, ("reseting coverageGenerator"));
  m_CoverageGenerator.reset();

  /// firstly stop all rendering commands in progress and collect all commands into queues

  for (unsigned i = 0; i < cpuCores; ++i)
    m_QueuedRenderer->PrepareQueueCancellation(i);

  m_TileRenderer->ClearCommands();
  m_TileRenderer->SetSequenceID(numeric_limits<int>::max());
  m_TileRenderer->CancelCommands();
  m_TileRenderer->WaitForEmptyAndFinished();

  /// now we should cancel all collected commands

  for (unsigned i = 0; i < cpuCores; ++i)
    m_QueuedRenderer->CancelQueuedCommands(i);

  LOG(LINFO, ("reseting tileRenderer"));
  m_TileRenderer.reset();
  LOG(LINFO, ("done reseting tileRenderer"));
}

void TilingRenderPolicyST::SetRenderFn(TRenderFn renderFn)
{
  int cpuCores = GetPlatform().CpuCores();
  string skinName = SkinName();

  graphics::PacketsQueue ** queues = new graphics::PacketsQueue*[cpuCores];

  for (unsigned i = 0; i < cpuCores; ++i)
    queues[i] = m_QueuedRenderer->GetPacketsQueue(i);

  m_TileRenderer.reset(new TileRenderer(TileSize(),
                                        skinName,
                                        Density(),
                                        cpuCores,
                                        m_bgColor,
                                        renderFn,
                                        m_primaryRC,
                                        m_resourceManager,
                                        VisualScale(),
                                        queues));

  delete [] queues;

  /// CoverageGenerator rendering queue could execute commands partially
  /// as there are no render-to-texture calls.
//  m_QueuedRenderer->SetPartialExecution(cpuCores, true);
  m_CoverageGenerator.reset(new CoverageGenerator(skinName,
                                                  Density(),
                                                  m_TileRenderer.get(),
                                                  m_windowHandle,
                                                  m_primaryRC,
                                                  m_resourceManager,
                                                  m_QueuedRenderer->GetPacketsQueue(cpuCores),
                                                  m_countryIndexFn));
}
