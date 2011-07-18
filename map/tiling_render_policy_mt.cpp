#include "../base/SRC_FIRST.hpp"

#include "drawer_yg.hpp"
#include "events.hpp"
#include "tiling_render_policy_mt.hpp"

#include "../geometry/screenbase.hpp"

#include "../platform/platform.hpp"


TilingRenderPolicyMT::TilingRenderPolicyMT(shared_ptr<WindowHandle> const & windowHandle,
                                           RenderPolicy::render_fn_t const & renderFn)
  : RenderPolicy(windowHandle, renderFn),
    m_renderQueue(GetPlatform().SkinName(),
                  GetPlatform().IsBenchmarking(),
                  GetPlatform().ScaleEtalonSize(),
                  GetPlatform().MaxTilesCount(),
                  GetPlatform().CpuCores(),
                  bgColor())
{
  m_renderQueue.AddWindowHandle(windowHandle);
}

void TilingRenderPolicyMT::Initialize(shared_ptr<yg::gl::RenderContext> const & primaryContext,
                                      shared_ptr<yg::ResourceManager> const & resourceManager)
{
  RenderPolicy::Initialize(primaryContext, resourceManager);
  m_renderQueue.InitializeGL(primaryContext, resourceManager, GetPlatform().VisualScale());
}

void TilingRenderPolicyMT::OnSize(int w, int h)
{
}

void TilingRenderPolicyMT::DrawFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & currentScreen)
{
  DrawerYG * pDrawer = e->drawer().get();
  pDrawer->screen()->clear(bgColor());

  m_infoLayer.clear();

  m_tiler.seed(currentScreen,
               currentScreen.GlobalRect().Center(),
               GetPlatform().TileSize(),
               GetPlatform().ScaleEtalonSize());

  while (m_tiler.hasTile())
  {
    yg::Tiler::RectInfo ri = m_tiler.nextTile();

    m_renderQueue.TileCache().lock();

    if (m_renderQueue.TileCache().hasTile(ri))
    {
      m_renderQueue.TileCache().touchTile(ri);
      yg::Tile tile = m_renderQueue.TileCache().getTile(ri);
      m_renderQueue.TileCache().unlock();

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
      m_renderQueue.TileCache().unlock();
      m_renderQueue.AddCommand(renderFn(), ri, m_tiler.seqNum());
    }
  }

  m_infoLayer.draw(pDrawer->screen().get(),
                   math::Identity<double, 3>());
}
