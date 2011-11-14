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

TilingRenderPolicyST::TilingRenderPolicyST(VideoTimer * videoTimer,
                                           DrawerYG::Params const & params,
                                           yg::ResourceManager::Params const & rmParams,
                                           shared_ptr<yg::gl::RenderContext> const & primaryRC)
  : RenderPolicy(primaryRC, true),
    m_tileCache(GetPlatform().MaxTilesCount() - 1),
    m_tiler(GetPlatform().TileSize(), GetPlatform().ScaleEtalonSize())
{
  yg::ResourceManager::Params rmp = rmParams;

  rmp.m_primaryStoragesParams = yg::ResourceManager::StoragePoolParams(50000 * sizeof(yg::gl::Vertex),
                                                                       10000 * sizeof(unsigned short),
                                                                       15,
                                                                       false);

  rmp.m_smallStoragesParams = yg::ResourceManager::StoragePoolParams(5000 * sizeof(yg::gl::Vertex),
                                                                     10000 * sizeof(unsigned short),
                                                                     100,
                                                                     false);

  rmp.m_blitStoragesParams = yg::ResourceManager::StoragePoolParams(10 * sizeof(yg::gl::AuxVertex),
                                                                    10 * sizeof(unsigned short),
                                                                    50,
                                                                    true);

  rmp.m_multiBlitStoragesParams = yg::ResourceManager::StoragePoolParams(500 * sizeof(yg::gl::AuxVertex),
                                                                         500 * sizeof(unsigned short),
                                                                         10,
                                                                         true);

  rmp.m_primaryTexturesParams = yg::ResourceManager::TexturePoolParams(512, 256, 10, rmp.m_rtFormat, true);

  rmp.m_fontTexturesParams = yg::ResourceManager::TexturePoolParams(512, 256, 5, rmp.m_rtFormat, true);

  rmp.m_renderTargetTexturesParams = yg::ResourceManager::TexturePoolParams(GetPlatform().TileSize(),
                                                                            GetPlatform().TileSize(),
                                                                            GetPlatform().MaxTilesCount(),
                                                                            rmp.m_rtFormat,
                                                                            true);

  rmp.m_glyphCacheParams = yg::ResourceManager::GlyphCacheParams("unicode_blocks.txt",
                                                                 "fonts_whitelist.txt",
                                                                 "fonts_blacklist.txt",
                                                                 2 * 1024 * 1024,
                                                                 GetPlatform().CpuCores() + 2,
                                                                 GetPlatform().CpuCores());


  rmp.m_isMergeable = false;
  rmp.m_useVA = !yg::gl::g_isBufferObjectsSupported;

  m_resourceManager.reset(new yg::ResourceManager(rmp));

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
  p.m_isSynchronized = true;

  m_drawer.reset(new DrawerYG(p));

  m_windowHandle.reset(new WindowHandle());

  m_windowHandle->setUpdatesEnabled(false);
  m_windowHandle->setVideoTimer(videoTimer);
  m_windowHandle->setRenderContext(primaryRC);

  /// render single tile on the same thread
  shared_ptr<yg::gl::FrameBuffer> frameBuffer(new yg::gl::FrameBuffer());

  unsigned tileWidth = m_resourceManager->params().m_renderTargetTexturesParams.m_texWidth;
  unsigned tileHeight = m_resourceManager->params().m_renderTargetTexturesParams.m_texHeight;

  shared_ptr<yg::gl::RenderBuffer> depthBuffer(new yg::gl::RenderBuffer(tileWidth, tileHeight, true));
  frameBuffer->setDepthBuffer(depthBuffer);

  p = DrawerYG::Params();

  p.m_resourceManager = m_resourceManager;
  p.m_frameBuffer = frameBuffer;
  p.m_glyphCacheID = m_resourceManager->guiThreadGlyphCacheID();
  p.m_threadID = 0;
  p.m_skinName = GetPlatform().SkinName();
  p.m_visualScale = GetPlatform().VisualScale();

  m_tileDrawer = make_shared_ptr(new DrawerYG(p));
  m_tileDrawer->onSize(tileWidth, tileHeight);

  m2::RectI renderRect(1, 1, tileWidth - 1, tileWidth - 1);
  m_tileScreen.OnSize(renderRect);
}

void TilingRenderPolicyST::DrawFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & currentScreen)
{
  DrawerYG * pDrawer = e->drawer();

  pDrawer->screen()->clear(m_bgColor);

  m_infoLayer.clear();

  m_tiler.seed(currentScreen,
               currentScreen.GlobalRect().GetGlobalRect().Center());

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
      shared_ptr<yg::gl::BaseTexture> tileTarget = m_resourceManager->renderTargetTextures()->Reserve();

      shared_ptr<yg::InfoLayer> tileInfoLayer(new yg::InfoLayer());

      m_tileDrawer->screen()->setRenderTarget(tileTarget);
      m_tileDrawer->screen()->setInfoLayer(tileInfoLayer);

      m_tileDrawer->beginFrame();

      yg::Color c = m_bgColor;

      m_tileDrawer->clear(yg::Color(c.r, c.g, c.b, 0));

      unsigned tileWidth = m_resourceManager->params().m_renderTargetTexturesParams.m_texWidth;
      unsigned tileHeight = m_resourceManager->params().m_renderTargetTexturesParams.m_texHeight;

      m2::RectI renderRect(1, 1, tileWidth - 1, tileHeight - 1);
      m_tileDrawer->screen()->setClipRect(renderRect);
      m_tileDrawer->clear(c);

      m_tileScreen.SetFromRect(m2::AnyRectD(ri.m_rect));

      m2::RectD selectRect;
      m2::RectD clipRect;

      double inflationSize = 24 * GetPlatform().VisualScale();

      m_tileScreen.PtoG(m2::RectD(renderRect), selectRect);
      m_tileScreen.PtoG(m2::Inflate(m2::RectD(renderRect), inflationSize, inflationSize), clipRect);

      m_renderFn(paintEvent,
                 m_tileScreen,
                 selectRect,
                 clipRect,
                 ri.m_drawScale);

      m_tileDrawer->endFrame();
      m_tileDrawer->screen()->resetInfoLayer();

      Tile tile(tileTarget, tileInfoLayer, m_tileScreen, ri, 0);
      m_tileCache.writeLock();
      m_tileCache.addTile(ri, TileCache::Entry(tile, m_resourceManager));
      m_tileCache.writeUnlock();

      m_tileCache.readLock();
      m_tileCache.touchTile(ri);
      tile = m_tileCache.getTile(ri);
      m_tileCache.readUnlock();

      pDrawer->screen()->blit(tile.m_renderTarget, tile.m_tileScreen, currentScreen, true,
                              yg::Color(),
                              m2::RectI(0, 0, tileWidth - 2, tileHeight - 2),
                              m2::RectU(1, 1, tileWidth - 1, tileHeight - 1));

      m_windowHandle->invalidate();
    }
  }
}

bool TilingRenderPolicyST::IsTiling() const
{
  return true;
}
