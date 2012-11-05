#include "tiling_render_policy_st.hpp"

#include "../platform/platform.hpp"

#include "../yg/internal/opengl.hpp"
#include "../yg/skin.hpp"

#include "window_handle.hpp"
#include "queued_renderer.hpp"
#include "tile_renderer.hpp"
#include "coverage_generator.hpp"

TilingRenderPolicyST::TilingRenderPolicyST(Params const & p)
  : BasicTilingRenderPolicy(p,
                            true)
{
  int cpuCores = GetPlatform().CpuCores();

  yg::ResourceManager::Params rmp = p.m_rmParams;

  rmp.checkDeviceCaps();

  rmp.m_primaryTexturesParams = yg::ResourceManager::TexturePoolParams(512,
                                                                       256,
                                                                       1,
                                                                       rmp.m_texFormat,
                                                                       true,
                                                                       true,
                                                                       true,
                                                                       1,
                                                                       "primaryTexture",
                                                                       true,
                                                                       true);

  rmp.m_fontTexturesParams = yg::ResourceManager::TexturePoolParams(256,
                                                                    256,
                                                                    10,
                                                                    rmp.m_texFormat,
                                                                    true,
                                                                    true,
                                                                    true,
                                                                    1,
                                                                    "fontTexture",
                                                                    true,
                                                                    true);

  rmp.m_primaryStoragesParams = yg::ResourceManager::StoragePoolParams(6000 * sizeof(yg::gl::Vertex),
                                                                       sizeof(yg::gl::Vertex),
                                                                       9000 * sizeof(unsigned short),
                                                                       sizeof(unsigned short),
                                                                       10,
                                                                       true,
                                                                       true,
                                                                       2,
                                                                       "primaryStorage",
                                                                       true,
                                                                       true);

  rmp.m_smallStoragesParams = yg::ResourceManager::StoragePoolParams(6000 * sizeof(yg::gl::Vertex),
                                                                     sizeof(yg::gl::Vertex),
                                                                     9000 * sizeof(unsigned short),
                                                                     sizeof(unsigned short),
                                                                     1,
                                                                     true,
                                                                     true,
                                                                     1,
                                                                     "smallStorage",
                                                                     true,
                                                                     true);

  rmp.m_multiBlitStoragesParams = yg::ResourceManager::StoragePoolParams(1500 * sizeof(yg::gl::Vertex),
                                                                         sizeof(yg::gl::Vertex),
                                                                         2500 * sizeof(unsigned short),
                                                                         sizeof(unsigned short),
                                                                         1,
                                                                         true,
                                                                         true,
                                                                         1,
                                                                         "multiBlitStorage",
                                                                         true,
                                                                         true);

  rmp.m_renderTargetTexturesParams = yg::ResourceManager::TexturePoolParams(TileSize(),
                                                                            TileSize(),
                                                                            1,
                                                                            rmp.m_texRtFormat,
                                                                            true,
                                                                            true,
                                                                            true,
                                                                            4,
                                                                            "renderTargetTexture",
                                                                            true,
                                                                            true);

  rmp.m_guiThreadStoragesParams = yg::ResourceManager::StoragePoolParams(2000 * sizeof(yg::gl::Vertex),
                                                                         sizeof(yg::gl::Vertex),
                                                                         4000 * sizeof(unsigned short),
                                                                         sizeof(unsigned short),
                                                                         5,
                                                                         true,
                                                                         true,
                                                                         1,
                                                                         "guiThreadStorage",
                                                                         true,
                                                                         true);

  rmp.m_guiThreadTexturesParams = yg::ResourceManager::TexturePoolParams(256,
                                                                    128,
                                                                    2,
                                                                    rmp.m_texFormat,
                                                                    true,
                                                                    true,
                                                                    true,
                                                                    1,
                                                                    "guiThreadTexture",
                                                                    true,
                                                                    true);

/*  bool * debuggingFlags = new bool[cpuCores + 2];
  for (unsigned i = 0; i < cpuCores + 2; ++i)
    debuggingFlags[i] = false;

  debuggingFlags[0] = true;*/

  rmp.m_glyphCacheParams = yg::ResourceManager::GlyphCacheParams("unicode_blocks.txt",
                                                                 "fonts_whitelist.txt",
                                                                 "fonts_blacklist.txt",
                                                                 2 * 1024 * 1024,
                                                                 cpuCores + 2,
                                                                 cpuCores,
                                                                 0);

//  delete [] debuggingFlags;

  rmp.m_useSingleThreadedOGL = true;
  rmp.fitIntoLimits();

  m_maxTilesCount = rmp.m_renderTargetTexturesParams.m_texCount;

  m_resourceManager.reset(new yg::ResourceManager(rmp));

  m_QueuedRenderer->SetSinglePipelineProcessing(m_resourceManager->useReadPixelsToSynchronize());

  Platform::FilesList fonts;
  GetPlatform().GetFontNames(fonts);
  m_resourceManager->addFonts(fonts);

  SetSkin(make_shared_ptr(yg::loadSkin(m_resourceManager, SkinName())));

  DrawerYG::Params dp;

  dp.m_frameBuffer = make_shared_ptr(new yg::gl::FrameBuffer(p.m_useDefaultFB));
  dp.m_resourceManager = m_resourceManager;
  dp.m_glyphCacheID = m_resourceManager->guiThreadGlyphCacheID();
  dp.m_skin = GetSkin();
  dp.m_visualScale = VisualScale();
  dp.m_useGuiResources = true;
  dp.m_isSynchronized = false;
  dp.m_fastSolidPath = true;
//  p.m_isDebugging = true;

  m_drawer.reset(new DrawerYG(dp));

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

  yg::gl::PacketsQueue ** queues = new yg::gl::PacketsQueue*[cpuCores];

  for (unsigned i = 0; i < cpuCores; ++i)
    queues[i] = m_QueuedRenderer->GetPacketsQueue(i);

  m_TileRenderer.reset(new TileRenderer(TileSize(),
                                        skinName,
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
                                                  m_TileRenderer.get(),
                                                  m_windowHandle,
                                                  m_primaryRC,
                                                  m_resourceManager,
                                                  m_QueuedRenderer->GetPacketsQueue(cpuCores),
                                                  m_countryNameFn));
}
