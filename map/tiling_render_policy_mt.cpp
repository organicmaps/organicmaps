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

TilingRenderPolicyMT::TilingRenderPolicyMT(VideoTimer * videoTimer,
                                           bool useDefaultFB,
                                           yg::ResourceManager::Params const & rmParams,
                                           shared_ptr<yg::gl::RenderContext> const & primaryRC)
  : RenderPolicy(primaryRC, false)
{
  yg::ResourceManager::Params rmp = rmParams;

  rmp.m_primaryStoragesParams = yg::ResourceManager::StoragePoolParams(50000 * sizeof(yg::gl::Vertex),
                                                                       sizeof(yg::gl::Vertex),
                                                                       100000 * sizeof(unsigned short),
                                                                       sizeof(unsigned short),
                                                                       15,
                                                                       false,
                                                                       true,
                                                                       1,
                                                                       "primaryStorage");


  rmp.m_multiBlitStoragesParams = yg::ResourceManager::StoragePoolParams(500 * sizeof(yg::gl::Vertex),
                                                                         sizeof(yg::gl::Vertex),
                                                                         500 * sizeof(unsigned short),
                                                                         sizeof(unsigned short),
                                                                         10,
                                                                         true,
                                                                         true,
                                                                         1,
                                                                         "multiBlitStorage");

  rmp.m_guiThreadStoragesParams = yg::ResourceManager::StoragePoolParams(5000 * sizeof(yg::gl::Vertex),
                                                                   sizeof(yg::gl::Vertex),
                                                                   10000 * sizeof(unsigned short),
                                                                   sizeof(unsigned short),
                                                                   10,
                                                                   true,
                                                                   true,
                                                                   1,
                                                                   "guiThreadStorage");

  rmp.m_primaryTexturesParams = yg::ResourceManager::TexturePoolParams(512,
                                                                       256,
                                                                       10,
                                                                       rmp.m_texFormat,
                                                                       true,
                                                                       true,
                                                                       true,
                                                                       1,
                                                                       "primaryTexture");

  rmp.m_fontTexturesParams = yg::ResourceManager::TexturePoolParams(512,
                                                                    256,
                                                                    2,
                                                                    rmp.m_texFormat,
                                                                    true,
                                                                    true,
                                                                    true,
                                                                    1,
                                                                    "fontTexture");

  rmp.m_renderTargetTexturesParams = yg::ResourceManager::TexturePoolParams(GetPlatform().TileSize(),
                                                                            GetPlatform().TileSize(),
                                                                            GetPlatform().MaxTilesCount(),
                                                                            rmp.m_texFormat,
                                                                            true,
                                                                            true,
                                                                            false,
                                                                            5,
                                                                            "renderTargetTexture");

  rmp.m_styleCacheTexturesParams = yg::ResourceManager::TexturePoolParams(rmp.m_fontTexturesParams.m_texWidth,
                                                                          rmp.m_fontTexturesParams.m_texHeight * 4,
                                                                          2,
                                                                          rmp.m_texFormat,
                                                                          true,
                                                                          true,
                                                                          true,
                                                                          1,
                                                                          "styleCacheTexture");

  rmp.m_guiThreadTexturesParams = yg::ResourceManager::TexturePoolParams(256,
                                                                    128,
                                                                    4,
                                                                    rmp.m_texFormat,
                                                                    true,
                                                                    true,
                                                                    true,
                                                                    1,
                                                                    "guiThreadTexture");

  rmp.m_glyphCacheParams = yg::ResourceManager::GlyphCacheParams("unicode_blocks.txt",
                                                                 "fonts_whitelist.txt",
                                                                 "fonts_blacklist.txt",
                                                                 2 * 1024 * 1024,
                                                                 GetPlatform().CpuCores() + 2,
                                                                 GetPlatform().CpuCores());

  rmp.m_useSingleThreadedOGL = false;
  rmp.m_useVA = !yg::gl::g_isBufferObjectsSupported;

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

  m_drawer.reset(new DrawerYG(p));

  m_windowHandle.reset(new WindowHandle());

  m_windowHandle->setUpdatesEnabled(false);
  m_windowHandle->setRenderPolicy(this);
  m_windowHandle->setVideoTimer(videoTimer);
  m_windowHandle->setRenderContext(primaryRC);
}

void TilingRenderPolicyMT::SetRenderFn(TRenderFn renderFn)
{
  RenderPolicy::SetRenderFn(renderFn);

  m_tileRenderer.reset(new TileRenderer(GetPlatform().SkinName(),
                                        GetPlatform().MaxTilesCount(),
                                        1, //GetPlatform().CpuCores(),
                                        m_bgColor,
                                        renderFn,
                                        m_primaryRC,
                                        m_resourceManager,
                                        GetPlatform().VisualScale(),
                                        0));

  m_coverageGenerator.reset(new CoverageGenerator(GetPlatform().TileSize(),
                                                  GetPlatform().ScaleEtalonSize(),
                                                  m_tileRenderer.get(),
                                                  m_windowHandle,
                                                  m_primaryRC,
                                                  m_resourceManager,
                                                  0));
}

void TilingRenderPolicyMT::BeginFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s)
{
}

void TilingRenderPolicyMT::EndFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s)
{
  ScreenCoverage * curCvg = &m_coverageGenerator->CurrentCoverage();
  curCvg->EndFrame(e->drawer()->screen().get());
  m_coverageGenerator->Mutex().Unlock();
}

void TilingRenderPolicyMT::DrawFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & currentScreen)
{
  DrawerYG * pDrawer = e->drawer();

  pDrawer->beginFrame();

  pDrawer->screen()->clear(m_bgColor);

  m_coverageGenerator->AddCoverScreenTask(currentScreen);

  m_coverageGenerator->Mutex().Lock();

  ScreenCoverage * curCvg = &m_coverageGenerator->CurrentCoverage();

  curCvg->Draw(pDrawer->screen().get(), currentScreen);

  m_drawScale = curCvg->GetDrawScale();

  pDrawer->endFrame();
}

int TilingRenderPolicyMT::GetDrawScale(ScreenBase const & s) const
{
  return m_drawScale;
}

TileRenderer & TilingRenderPolicyMT::GetTileRenderer()
{
  return *m_tileRenderer.get();
}

void TilingRenderPolicyMT::StartScale()
{
  m_isScaling = true;
}

void TilingRenderPolicyMT::StopScale()
{
  m_isScaling = false;
}

bool TilingRenderPolicyMT::IsTiling() const
{
  return true;
}
