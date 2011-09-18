#include "../base/SRC_FIRST.hpp"

#include "tiling_render_policy_st.hpp"

#include "drawer_yg.hpp"
#include "window_handle.hpp"
#include "events.hpp"

#include "../yg/framebuffer.hpp"
#include "../yg/renderbuffer.hpp"
#include "../yg/resource_manager.hpp"
#include "../yg/base_texture.hpp"

#include "../platform/platform.hpp"


TilingRenderPolicyST::TilingRenderPolicyST(shared_ptr<WindowHandle> const & windowHandle,
                                           RenderPolicy::TRenderFn const & renderFn)
  : RenderPolicy(windowHandle, renderFn),
    m_tileCache(GetPlatform().MaxTilesCount() - 1),
    m_tiler(GetPlatform().TileSize(), GetPlatform().ScaleEtalonSize())
{}

void TilingRenderPolicyST::Initialize(shared_ptr<yg::gl::RenderContext> const & primaryContext,
                                      shared_ptr<yg::ResourceManager> const & rm)
{
  RenderPolicy::Initialize(primaryContext, rm);

  rm->initRenderTargets(GetPlatform().TileSize(), GetPlatform().TileSize(), GetPlatform().MaxTilesCount());

  /// render single tile on the same thread
  shared_ptr<yg::gl::FrameBuffer> frameBuffer(new yg::gl::FrameBuffer());

  unsigned tileWidth = resourceManager()->tileTextureWidth();
  unsigned tileHeight = resourceManager()->tileTextureHeight();

  shared_ptr<yg::gl::RenderBuffer> depthBuffer(new yg::gl::RenderBuffer(tileWidth, tileHeight, true));
  frameBuffer->setDepthBuffer(depthBuffer);

  DrawerYG::params_t params;

  params.m_resourceManager = resourceManager();
  params.m_frameBuffer = frameBuffer;
  params.m_glyphCacheID = resourceManager()->guiThreadGlyphCacheID();
  params.m_threadID = 0;
  params.m_skinName = GetPlatform().SkinName();
  params.m_visualScale = GetPlatform().VisualScale();

  m_tileDrawer = make_shared_ptr(new DrawerYG(params));
  m_tileDrawer->onSize(tileWidth, tileHeight);

  m2::RectI renderRect(1, 1, tileWidth - 1, tileWidth - 1);
  m_tileScreen.OnSize(renderRect);
}

void TilingRenderPolicyST::DrawFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & currentScreen)
{
  DrawerYG * pDrawer = e->drawer();

  pDrawer->screen()->clear(bgColor());

  m_infoLayer.clear();

  m_tiler.seed(currentScreen,
               currentScreen.GlobalRect().Center());

  vector<Tiler::RectInfo> visibleTiles;
  m_tiler.visibleTiles(visibleTiles);

  for (unsigned i = 0; i < visibleTiles.size(); ++i)
  {
    Tiler::RectInfo ri = visibleTiles[i];

    m_tileCache.readLock();

    if (m_tileCache.hasTile(ri))
    {
      m_tileCache.touchTile(ri);
      Tile tile = m_tileCache.getTile(ri);
      m_tileCache.readUnlock();

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
      m_tileCache.readUnlock();
      shared_ptr<PaintEvent> paintEvent(new PaintEvent(m_tileDrawer.get()));
      shared_ptr<yg::gl::BaseTexture> tileTarget = resourceManager()->renderTargets()->Reserve();

      shared_ptr<yg::InfoLayer> tileInfoLayer(new yg::InfoLayer());

      m_tileDrawer->screen()->setRenderTarget(tileTarget);
      m_tileDrawer->screen()->setInfoLayer(tileInfoLayer);

      m_tileDrawer->beginFrame();

      yg::Color c = bgColor();

      m_tileDrawer->clear(yg::Color(c.r, c.g, c.b, 0));
      m2::RectI renderRect(1, 1, resourceManager()->tileTextureWidth() - 1, resourceManager()->tileTextureHeight() - 1);
      m_tileDrawer->screen()->setClipRect(renderRect);
      m_tileDrawer->clear(c);

      m_tileScreen.SetFromRect(ri.m_rect);

      m2::RectD selectRect;
      m2::RectD clipRect;

      double inflationSize = 24 * GetPlatform().VisualScale();

      m_tileScreen.PtoG(m2::RectD(renderRect), selectRect);
      m_tileScreen.PtoG(m2::Inflate(m2::RectD(renderRect), inflationSize, inflationSize), clipRect);

      renderFn()(paintEvent,
                m_tileScreen,
                selectRect,
                clipRect,
                ri.m_drawScale);

      m_tileDrawer->endFrame();
      m_tileDrawer->screen()->resetInfoLayer();

      Tile tile(tileTarget, tileInfoLayer, m_tileScreen, ri, 0);
      m_tileCache.writeLock();
      m_tileCache.addTile(ri, TileCache::Entry(tile, resourceManager()));
      m_tileCache.writeUnlock();

      m_tileCache.readLock();
      m_tileCache.touchTile(ri);
      tile = m_tileCache.getTile(ri);
      m_tileCache.readUnlock();

      size_t tileWidth = tile.m_renderTarget->width();
      size_t tileHeight = tile.m_renderTarget->height();

      pDrawer->screen()->blit(tile.m_renderTarget, tile.m_tileScreen, currentScreen, true,
                              yg::Color(),
                              m2::RectI(0, 0, tileWidth - 2, tileHeight - 2),
                              m2::RectU(1, 1, tileWidth - 1, tileHeight - 1));

      windowHandle()->invalidate();
    }
  }
}
