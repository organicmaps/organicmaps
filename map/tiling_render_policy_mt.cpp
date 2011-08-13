#include "../base/SRC_FIRST.hpp"

#include "../platform/platform.hpp"
#include "../std/bind.hpp"

#include "../geometry/screenbase.hpp"

#include "../yg/base_texture.hpp"

#include "drawer_yg.hpp"
#include "events.hpp"
#include "tiling_render_policy_mt.hpp"
#include "window_handle.hpp"

TilingRenderPolicyMT::TilingRenderPolicyMT(shared_ptr<WindowHandle> const & windowHandle,
                                           RenderPolicy::TRenderFn const & renderFn)
  : RenderPolicy(windowHandle, renderFn),
    m_renderQueue(GetPlatform().SkinName(),
                  GetPlatform().IsBenchmarking(),
                  GetPlatform().ScaleEtalonSize(),
                  GetPlatform().MaxTilesCount(),
                  GetPlatform().CpuCores(),
                  bgColor()),
    m_tiler(GetPlatform().TileSize(),
            GetPlatform().ScaleEtalonSize())
//    m_coverageGenerator(GetPlatform().TileSize(),
//                        GetPlatform().ScaleEtalonSize(),
//                        renderFn,
//                        &m_renderQueue)
{
  m_renderQueue.AddWindowHandle(windowHandle);
}

void TilingRenderPolicyMT::Initialize(shared_ptr<yg::gl::RenderContext> const & primaryContext,
                                      shared_ptr<yg::ResourceManager> const & resourceManager)
{
  RenderPolicy::Initialize(primaryContext, resourceManager);
  m_renderQueue.Initialize(primaryContext, resourceManager, GetPlatform().VisualScale());
//  m_coverageGenerator.Initialize();
}

void TilingRenderPolicyMT::OnSize(int /*w*/, int /*h*/)
{
}

void TilingRenderPolicyMT::DrawFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & currentScreen)
{
  DrawerYG * pDrawer = e->drawer().get();
  pDrawer->screen()->clear(bgColor());

/*  m_coverageGenerator.AddCoverageTask(currentScreen);

  ScreenCoverage coverage;

  {
    /// copying full coverage
    threads::MutexGuard(m_coverageGenerator.Mutex());
    coverage = *m_coverageGenerator.CurrentCoverage();
  }

  /// drawing from full coverage
  for (unsigned i = 0; i < coverage.m_tiles.size(); ++i)
  {
    shared_ptr<Tile const> tile = coverage.m_tiles[i];

    size_t tileWidth = tile->m_renderTarget->width();
    size_t tileHeight = tile->m_renderTarget->height();

    pDrawer->screen()->blit(tile->m_renderTarget, tile->m_tileScreen, currentScreen, true,
                            yg::Color(),
                            m2::RectI(0, 0, tileWidth - 2, tileHeight - 2),
                            m2::RectU(1, 1, tileWidth - 1, tileHeight - 1));
  }

  coverage.m_infoLayer.draw(pDrawer->screen().get(),
                            coverage.m_screen.PtoGMatrix() * currentScreen.GtoPMatrix());

  coverage.Clear();*/

  m_infoLayer.clear();

  m_tiler.seed(currentScreen,
               currentScreen.GlobalRect().Center());

  while (m_tiler.hasTile())
  {
    Tiler::RectInfo ri = m_tiler.nextTile();

    TileCache & tileCache = m_renderQueue.GetTileCache();

    tileCache.lock();

    if (tileCache.hasTile(ri))
    {
      tileCache.touchTile(ri);
      Tile tile = tileCache.getTile(ri);
      tileCache.unlock();

      size_t tileWidth = tile.m_renderTarget->width();
      size_t tileHeight = tile.m_renderTarget->height();

      pDrawer->screen()->blit(tile.m_renderTarget, tile.m_tileScreen, currentScreen, true,
                              yg::Color(),
                              m2::RectI(0, 0, tileWidth - 2, tileHeight - 2),
                              m2::RectU(1, 1, tileWidth - 1, tileHeight - 1));

      m_infoLayer.merge(*tile.m_infoLayer.get(), tile.m_tileScreen.PtoGMatrix() * currentScreen.GtoPMatrix());
    }
    else
    {
      tileCache.unlock();
      m_renderQueue.AddCommand(renderFn(), ri, m_tiler.sequenceID(), bind(&WindowHandle::invalidate, windowHandle()));
    }
  }

  m_infoLayer.draw(pDrawer->screen().get(),
                   math::Identity<double, 3>());
}
