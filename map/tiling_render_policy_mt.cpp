#include "../base/SRC_FIRST.hpp"

#include "../platform/platform.hpp"
#include "../std/bind.hpp"

#include "../geometry/screenbase.hpp"

#include "../yg/base_texture.hpp"

#include "drawer_yg.hpp"
#include "events.hpp"
#include "tiling_render_policy_mt.hpp"
#include "window_handle.hpp"
#include "screen_coverage.hpp"

TilingRenderPolicyMT::TilingRenderPolicyMT(shared_ptr<WindowHandle> const & windowHandle,
                                           RenderPolicy::TRenderFn const & renderFn)
  : RenderPolicy(windowHandle, renderFn),
    m_tileRenderer(GetPlatform().SkinName(),
                  GetPlatform().MaxTilesCount(),
                  1, //GetPlatform().CpuCores(),
                  bgColor(),
                  renderFn),
    m_coverageGenerator(GetPlatform().TileSize(),
                        GetPlatform().ScaleEtalonSize(),
                        &m_tileRenderer,
                        windowHandle)
{

}

void TilingRenderPolicyMT::Initialize(shared_ptr<yg::gl::RenderContext> const & primaryContext,
                                      shared_ptr<yg::ResourceManager> const & resourceManager)
{
  RenderPolicy::Initialize(primaryContext, resourceManager);

  resourceManager->initRenderTargets(GetPlatform().TileSize(), GetPlatform().TileSize(), GetPlatform().MaxTilesCount());

  m_tileRenderer.Initialize(primaryContext, resourceManager, GetPlatform().VisualScale());
  m_coverageGenerator.Initialize();
}

void TilingRenderPolicyMT::DrawFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & currentScreen)
{
  DrawerYG * pDrawer = e->drawer();
  pDrawer->screen()->clear(bgColor());

  m_coverageGenerator.AddCoverScreenTask(currentScreen);

  {
    threads::MutexGuard g(m_coverageGenerator.Mutex());
    m_coverageGenerator.CurrentCoverage().Draw(pDrawer->screen().get(), currentScreen);
  }
}

TileRenderer & TilingRenderPolicyMT::GetTileRenderer()
{
  return m_tileRenderer;
}
