#include "../base/SRC_FIRST.hpp"

#include "tiling_render_policy_st.hpp"

#include "drawer_yg.hpp"
#include "window_handle.hpp"
#include "events.hpp"

#include "../yg/framebuffer.hpp"
#include "../yg/renderbuffer.hpp"
#include "../yg/resource_manager.hpp"
#include "../yg/base_texture.hpp"
#include "../yg/internal/opengl.hpp"

#include "../platform/platform.hpp"

#include "../base/SRC_FIRST.hpp"

#include "../platform/platform.hpp"
#include "../std/bind.hpp"

#include "../geometry/screenbase.hpp"

#include "../yg/base_texture.hpp"
#include "../yg/internal/opengl.hpp"

#include "drawer_yg.hpp"
#include "events.hpp"
#include "tiling_render_policy_mt.hpp"
#include "window_handle.hpp"
#include "screen_coverage.hpp"

TilingRenderPolicyST::TilingRenderPolicyST(VideoTimer * videoTimer,
                                           bool useDefaultFB,
                                           yg::ResourceManager::Params const & rmParams,
                                           shared_ptr<yg::gl::RenderContext> const & primaryRC)
  : QueuedRenderPolicy(GetPlatform().CpuCores() + 1, primaryRC, false),
    m_drawScale(0)
{
  yg::ResourceManager::Params rmp = rmParams;

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
                                                                            GetPlatform().MaxTilesCount(),
                                                                            rmp.m_texFormat,
                                                                            true,
                                                                            true,
                                                                            false,
                                                                            4,
                                                                            "renderTargetTexture",
                                                                            false,
                                                                            false);

  rmp.m_styleCacheTexturesParams = yg::ResourceManager::TexturePoolParams(1024,
                                                                          512,
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
  LOG(LINFO, ("deleting TilingRenderPolicyST"));

  base_t::CancelQueuedCommands(GetPlatform().CpuCores());

  LOG(LINFO, ("reseting coverageGenerator"));
  m_coverageGenerator.reset();

  for (unsigned i = 0; i < GetPlatform().CpuCores(); ++i)
    base_t::CancelQueuedCommands(i);

  LOG(LINFO, ("reseting tileRenderer"));
  m_tileRenderer.reset();
  LOG(LINFO, ("done reseting tileRenderer"));
}

void TilingRenderPolicyST::SetRenderFn(TRenderFn renderFn)
{
  RenderPolicy::SetRenderFn(renderFn);

  yg::gl::PacketsQueue ** queues = new yg::gl::PacketsQueue*[GetPlatform().CpuCores()];

  for (unsigned i = 0; i < GetPlatform().CpuCores(); ++i)
    queues[i] = base_t::GetPacketsQueue(i);

  m_tileRenderer.reset(new TileRenderer(GetPlatform().SkinName(),
                                        m_maxTilesCount,
                                        GetPlatform().CpuCores(),
                                        m_bgColor,
                                        renderFn,
                                        m_primaryRC,
                                        m_resourceManager,
                                        GetPlatform().VisualScale(),
                                        queues));

  delete [] queues;

  m_coverageGenerator.reset(new CoverageGenerator(GetPlatform().TileSize(),
                                                  GetPlatform().ScaleEtalonSize(),
                                                  m_tileRenderer.get(),
                                                  m_windowHandle,
                                                  m_primaryRC,
                                                  m_resourceManager,
                                                  base_t::GetPacketsQueue(GetPlatform().CpuCores())
                                                  ));


}

void TilingRenderPolicyST::EndFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s)
{
  ScreenCoverage * curCvg = &m_coverageGenerator->CurrentCoverage();
  curCvg->EndFrame(e->drawer()->screen().get());
  m_coverageGenerator->Mutex().Unlock();

  base_t::EndFrame(e, s);
}

void TilingRenderPolicyST::DrawFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & currentScreen)
{
  base_t::DrawFrame(e, currentScreen);

//  yg::gl::g_doLogOGLCalls = true;

  DrawerYG * pDrawer = e->drawer();

  pDrawer->beginFrame();

  pDrawer->screen()->clear(m_bgColor);

  m_coverageGenerator->AddCoverScreenTask(currentScreen);

  m_coverageGenerator->Mutex().Lock();

  ScreenCoverage * curCvg = &m_coverageGenerator->CurrentCoverage();

  curCvg->Draw(pDrawer->screen().get(), currentScreen);

  m_drawScale = curCvg->GetDrawScale();

  pDrawer->endFrame();

//  yg::gl::g_doLogOGLCalls = false;

  m_resourceManager->updatePoolState();
}

int TilingRenderPolicyST::GetDrawScale(ScreenBase const & s) const
{
  return m_drawScale;
}

TileRenderer & TilingRenderPolicyST::GetTileRenderer()
{
  return *m_tileRenderer.get();
}

void TilingRenderPolicyST::StartScale()
{
  m_isScaling = true;
}

void TilingRenderPolicyST::StopScale()
{
  m_isScaling = false;
}

bool TilingRenderPolicyST::IsTiling() const
{
  return true;
}

