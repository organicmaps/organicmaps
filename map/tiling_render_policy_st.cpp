#include "tiling_render_policy_st.hpp"

#include "../platform/platform.hpp"

#include "../yg/internal/opengl.hpp"

#include "window_handle.hpp"
#include "queued_renderer.hpp"
#include "tile_renderer.hpp"
#include "coverage_generator.hpp"

TilingRenderPolicyST::TilingRenderPolicyST(VideoTimer * videoTimer,
                                           bool useDefaultFB,
                                           yg::ResourceManager::Params const & rmParams,
                                           shared_ptr<yg::gl::RenderContext> const & primaryRC)
  : BasicTilingRenderPolicy(primaryRC,
                            false,
                            GetPlatform().CpuCores(),
                            make_shared_ptr(new QueuedRenderer(GetPlatform().CpuCores() + 1)))
{
  yg::ResourceManager::Params rmp = rmParams;

  rmp.selectTexRTFormat();

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

  rmp.m_primaryStoragesParams = yg::ResourceManager::StoragePoolParams(5000 * sizeof(yg::gl::Vertex),
                                                                       sizeof(yg::gl::Vertex),
                                                                       10000 * sizeof(unsigned short),
                                                                       sizeof(unsigned short),
                                                                       10,
                                                                       true,
                                                                       true,
                                                                       2,
                                                                       "primaryStorage",
                                                                       true,
                                                                       true);

  rmp.m_multiBlitStoragesParams = yg::ResourceManager::StoragePoolParams(1500 * sizeof(yg::gl::Vertex),
                                                                         sizeof(yg::gl::Vertex),
                                                                         3000 * sizeof(unsigned short),
                                                                         sizeof(unsigned short),
                                                                         10,
                                                                         true,
                                                                         true,
                                                                         1,
                                                                         "multiBlitStorage",
                                                                         false,
                                                                         false);

  rmp.m_renderTargetTexturesParams = yg::ResourceManager::TexturePoolParams(GetPlatform().TileSize(),
                                                                            GetPlatform().TileSize(),
                                                                            GetPlatform().CpuCores(),
                                                                            rmp.m_texRtFormat,
                                                                            true,
                                                                            true,
                                                                            true,
                                                                            4,
                                                                            "renderTargetTexture",
                                                                            false,
                                                                            true);

  rmp.m_styleCacheTexturesParams = yg::ResourceManager::TexturePoolParams(512,
                                                                          512 * int(ceil(GetPlatform().VisualScale())),
                                                                          2,
                                                                          rmp.m_texFormat,
                                                                          true,
                                                                          true,
                                                                          true,
                                                                          1,
                                                                          "styleCacheTexture",
                                                                          false,
                                                                          false);

  rmp.m_guiThreadStoragesParams = yg::ResourceManager::StoragePoolParams(2000 * sizeof(yg::gl::Vertex),
                                                                         sizeof(yg::gl::Vertex),
                                                                         4000 * sizeof(unsigned short),
                                                                         sizeof(unsigned short),
                                                                         20,
                                                                         true,
                                                                         true,
                                                                         1,
                                                                         "guiThreadStorage",
                                                                         true,
                                                                         false);

  rmp.m_guiThreadTexturesParams = yg::ResourceManager::TexturePoolParams(256,
                                                                    128,
                                                                    4,
                                                                    rmp.m_texFormat,
                                                                    true,
                                                                    true,
                                                                    true,
                                                                    1,
                                                                    "guiThreadTexture",
                                                                    false,
                                                                    false);

/*  bool * debuggingFlags = new bool[GetPlatform().CpuCores() + 2];
  for (unsigned i = 0; i < GetPlatform().CpuCores() + 2; ++i)
    debuggingFlags[i] = false;

  debuggingFlags[0] = true;*/

  rmp.m_glyphCacheParams = yg::ResourceManager::GlyphCacheParams("unicode_blocks.txt",
                                                                 "fonts_whitelist.txt",
                                                                 "fonts_blacklist.txt",
                                                                 2 * 1024 * 1024,
                                                                 GetPlatform().CpuCores() + 2,
                                                                 GetPlatform().CpuCores(),
                                                                 0);

//  delete [] debuggingFlags;

  rmp.m_useSingleThreadedOGL = true;
  rmp.m_useVA = !yg::gl::g_isBufferObjectsSupported;

  rmp.fitIntoLimits();

  m_maxTilesCount = rmp.m_renderTargetTexturesParams.m_texCount;

  m_resourceManager.reset(new yg::ResourceManager(rmp));

  Platform::FilesList fonts;
  GetPlatform().GetFontNames(fonts);
  m_resourceManager->addFonts(fonts);

  DrawerYG::params_t p;

  p.m_frameBuffer = make_shared_ptr(new yg::gl::FrameBuffer(useDefaultFB));
  p.m_resourceManager = m_resourceManager;
  p.m_dynamicPagesCount = 2;
  p.m_textPagesCount = 2;
  p.m_glyphCacheID = m_resourceManager->guiThreadGlyphCacheID();
  p.m_skinName = GetPlatform().SkinName();
  p.m_visualScale = GetPlatform().VisualScale();
  p.m_useGuiResources = true;
  p.m_isSynchronized = false;
//  p.m_isDebugging = true;

  m_drawer.reset(new DrawerYG(p));

  m_windowHandle.reset(new WindowHandle());

  m_windowHandle->setUpdatesEnabled(false);
  m_windowHandle->setRenderPolicy(this);
  m_windowHandle->setVideoTimer(videoTimer);
  m_windowHandle->setRenderContext(primaryRC);
}

TilingRenderPolicyST::~TilingRenderPolicyST()
{
  LOG(LINFO, ("cancelling ResourceManager"));
  m_resourceManager->cancel();

  LOG(LINFO, ("deleting TilingRenderPolicyST"));

  m_QueuedRenderer->CancelQueuedCommands(GetPlatform().CpuCores());

  LOG(LINFO, ("reseting coverageGenerator"));
  m_CoverageGenerator.reset();

  /// firstly stop all rendering commands in progress and collect all commands into queues

  for (unsigned i = 0; i < GetPlatform().CpuCores(); ++i)
    m_QueuedRenderer->PrepareQueueCancellation(i);

  m_TileRenderer->ClearCommands();
  m_TileRenderer->SetSequenceID(numeric_limits<int>::max());
  m_TileRenderer->CancelCommands();
  m_TileRenderer->WaitForEmptyAndFinished();

  /// now we should cancel all collected commands

  for (unsigned i = 0; i < GetPlatform().CpuCores(); ++i)
    m_QueuedRenderer->CancelQueuedCommands(i);

  LOG(LINFO, ("reseting tileRenderer"));
  m_TileRenderer.reset();
  LOG(LINFO, ("done reseting tileRenderer"));
}

void TilingRenderPolicyST::SetRenderFn(TRenderFn renderFn)
{
  yg::gl::PacketsQueue ** queues = new yg::gl::PacketsQueue*[GetPlatform().CpuCores()];

  for (unsigned i = 0; i < GetPlatform().CpuCores(); ++i)
    queues[i] = m_QueuedRenderer->GetPacketsQueue(i);

  m_TileRenderer.reset(new TileRenderer(GetPlatform().SkinName(),
                                        GetPlatform().CpuCores(),
                                        m_bgColor,
                                        renderFn,
                                        m_primaryRC,
                                        m_resourceManager,
                                        GetPlatform().VisualScale(),
                                        queues));

  delete [] queues;

  m_CoverageGenerator.reset(new CoverageGenerator(m_TileRenderer.get(),
                                                  m_windowHandle,
                                                  m_primaryRC,
                                                  m_resourceManager,
                                                  m_QueuedRenderer->GetPacketsQueue(GetPlatform().CpuCores())
                                                  ));


}

