#include "../base/SRC_FIRST.hpp"

#include "tiling_render_policy_st.hpp"

#include "drawer_yg.hpp"
#include "window_handle.hpp"
#include "events.hpp"

#include "../yg/framebuffer.hpp"
#include "../yg/renderbuffer.hpp"
#include "../yg/resource_manager.hpp"

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
  params.m_useOverlay = true;
  params.m_threadID = 0;

  m_tileDrawer = make_shared_ptr(new DrawerYG(GetPlatform().SkinName(), params));
  m_tileDrawer->onSize(tileWidth, tileHeight);

  m_tileDrawer->SetVisualScale(GetPlatform().VisualScale());

  m2::RectI renderRect(1, 1, tileWidth - 1, tileWidth - 1);
  m_tileScreen.OnSize(renderRect);
}

void TilingRenderPolicyST::OnSize(int /*w*/, int /*h*/)
{}

void TilingRenderPolicyST::DrawFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & currentScreen)
{
  DrawerYG * pDrawer = e->drawer().get();

  pDrawer->screen()->clear(bgColor());

  m_infoLayer.clear();

  m_tiler.seed(currentScreen,
               currentScreen.GlobalRect().Center());

  while (m_tiler.hasTile())
  {
    yg::Tiler::RectInfo ri = m_tiler.nextTile();

    m_tileCache.lock();

    if (m_tileCache.hasTile(ri))
    {
      m_tileCache.touchTile(ri);
      yg::Tile tile = m_tileCache.getTile(ri);
      m_tileCache.unlock();

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
      m_tileCache.unlock();
      shared_ptr<PaintEvent> paintEvent(new PaintEvent(m_tileDrawer));
      shared_ptr<yg::gl::BaseTexture> tileTarget = resourceManager()->renderTargets().Front(true);

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

      m2::RectD selectionRect;

      double inflationSize = 24 * GetPlatform().VisualScale();

      m_tileScreen.PtoG(m2::Inflate(m2::RectD(renderRect), inflationSize, inflationSize), selectionRect);

      renderFn()(paintEvent,
                m_tileScreen,
                selectionRect,
                ri.m_drawScale);

      m_tileDrawer->endFrame();
      m_tileDrawer->screen()->resetInfoLayer();

      yg::Tile tile(tileTarget, tileInfoLayer, m_tileScreen, ri, 0);
      m_tileCache.lock();
      m_tileCache.addTile(ri, yg::TileCache::Entry(tile, resourceManager()));
      m_tileCache.unlock();

      m_tileCache.lock();
      m_tileCache.touchTile(ri);
      tile = m_tileCache.getTile(ri);
      m_tileCache.unlock();

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
