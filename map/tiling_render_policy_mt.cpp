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
                                           DrawerYG::Params const & params,
                                           shared_ptr<yg::gl::RenderContext> const & primaryRC)
  : RenderPolicy(primaryRC, true)
{
  m_resourceManager = make_shared_ptr(new yg::ResourceManager(
      50000 * sizeof(yg::gl::Vertex),
      100000 * sizeof(unsigned short),
      15,
      5000 * sizeof(yg::gl::Vertex),
      10000 * sizeof(unsigned short),
      100,
      10 * sizeof(yg::gl::AuxVertex),
      10 * sizeof(unsigned short),
      50,
      512, 256,
      10,
      512, 256,
      5,
      "unicode_blocks.txt",
      "fonts_whitelist.txt",
      "fonts_blacklist.txt",
      2 * 1024 * 1024,
      GetPlatform().CpuCores() + 2,
      yg::Rt8Bpp,
      !yg::gl::g_isBufferObjectsSupported));

  m_resourceManager->initMultiBlitStorage(500 * sizeof(yg::gl::AuxVertex), 500 * sizeof(unsigned short), 10);
  m_resourceManager->initRenderTargets(GetPlatform().TileSize(), GetPlatform().TileSize(), GetPlatform().MaxTilesCount());
  m_resourceManager->initStyleCacheTextures(m_resourceManager->fontTextureWidth(), m_resourceManager->fontTextureHeight() * 2, 2);
  m_resourceManager->initTinyStorage(300 * sizeof(yg::gl::Vertex), 600 * sizeof(unsigned short), 50);

  Platform::FilesList fonts;
  GetPlatform().GetFontNames(fonts);
  m_resourceManager->addFonts(fonts);

  DrawerYG::params_t p = params;

  p.m_resourceManager = m_resourceManager;
  p.m_dynamicPagesCount = 2;
  p.m_textPagesCount = 2;
  p.m_glyphCacheID = m_resourceManager->guiThreadGlyphCacheID();
  p.m_skinName = GetPlatform().SkinName();
  p.m_visualScale = GetPlatform().VisualScale();
  p.m_useTinyStorage = true;
  p.m_isSynchronized = false;

  m_drawer.reset(new DrawerYG(p));

  m_windowHandle.reset(new WindowHandle());

  m_windowHandle->setUpdatesEnabled(false);
  m_windowHandle->setVideoTimer(make_shared_ptr(videoTimer));
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
                                        GetPlatform().VisualScale()));

  m_coverageGenerator.reset(new CoverageGenerator(GetPlatform().TileSize(),
                                                  GetPlatform().ScaleEtalonSize(),
                                                  m_tileRenderer.get(),
                                                  m_windowHandle,
                                                  m_primaryRC,
                                                  m_resourceManager
                                                  ));
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

  pDrawer->screen()->clear(m_bgColor);

  m_coverageGenerator->AddCoverScreenTask(currentScreen);

  m_coverageGenerator->Mutex().Lock();

  ScreenCoverage * curCvg = &m_coverageGenerator->CurrentCoverage();

  curCvg->Draw(pDrawer->screen().get(), currentScreen);
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
