#include "../base/SRC_FIRST.hpp"

#include "tile_renderer.hpp"
#include "window_handle.hpp"

#include "../yg/render_state.hpp"
#include "../yg/rendercontext.hpp"
#include "../yg/base_texture.hpp"

#include "../std/bind.hpp"

#include "../base/logging.hpp"
#include "../base/condition.hpp"

TileRenderer::TileRenderer(
    string const & skinName,
    unsigned scaleEtalonSize,
    unsigned maxTilesCount,
    unsigned executorsCount,
    yg::Color const & bgColor,
    RenderPolicy::TRenderFn const & renderFn
  ) : m_queue(executorsCount),
      m_tileCache(maxTilesCount - executorsCount - 1),
      m_renderFn(renderFn),
      m_skinName(skinName),
      m_bgColor(bgColor),
      m_sequenceID(0)
{
}

void TileRenderer::Initialize(shared_ptr<yg::gl::RenderContext> const & primaryContext,
                             shared_ptr<yg::ResourceManager> const & resourceManager,
                             double visualScale)
{
  m_resourceManager = resourceManager;

  LOG(LINFO, ("initializing ", m_queue.ExecutorsCount(), " rendering threads"));

  int tileWidth = m_resourceManager->tileTextureWidth();
  int tileHeight = m_resourceManager->tileTextureHeight();

  for (unsigned i = 0; i < m_queue.ExecutorsCount(); ++i)
  {
    DrawerYG::params_t params;

    params.m_resourceManager = m_resourceManager;
    params.m_frameBuffer = make_shared_ptr(new yg::gl::FrameBuffer());

    shared_ptr<yg::gl::RenderBuffer> depthBuffer(new yg::gl::RenderBuffer(tileWidth, tileHeight, true));
    params.m_frameBuffer->setDepthBuffer(depthBuffer);

    params.m_glyphCacheID = m_resourceManager->renderThreadGlyphCacheID(i);
    params.m_useOverlay = true;
    params.m_threadID = i;
    params.m_visualScale = visualScale;
    params.m_skinName = m_skinName;
  /*  params.m_isDebugging = true;
    params.m_drawPathes = false;
    params.m_drawAreas = false;
    params.m_drawTexts = false; */

    m_drawerParams.push_back(params);
    m_drawers.push_back(0);
    m_renderContexts.push_back(primaryContext->createShared());
  }

  m_queue.AddInitCommand(bind(&TileRenderer::InitializeThreadGL, this, _1));
  m_queue.AddFinCommand(bind(&TileRenderer::FinalizeThreadGL, this, _1));

  m_queue.Start();
}

TileRenderer::~TileRenderer()
{
  m_queue.Cancel();
}

void TileRenderer::InitializeThreadGL(core::CommandsQueue::Environment const & env)
{
  m_renderContexts[env.GetThreadNum()]->makeCurrent();

  m_drawers[env.GetThreadNum()] = new DrawerYG(m_drawerParams[env.GetThreadNum()]);
}

void TileRenderer::FinalizeThreadGL(core::CommandsQueue::Environment const & env)
{
  m_renderContexts[env.GetThreadNum()]->endThreadDrawing();
  if (m_drawers[env.GetThreadNum()] != 0)
    delete m_drawers[env.GetThreadNum()];
}

void TileRenderer::DrawTile(core::CommandsQueue::Environment const & env,
                           Tiler::RectInfo const & rectInfo,
                           int sequenceID)
{
  DrawerYG * drawer = m_drawers[env.GetThreadNum()];

  ScreenBase frameScreen;

  unsigned tileWidth = m_resourceManager->tileTextureWidth();
  unsigned tileHeight = m_resourceManager->tileTextureHeight();

  m2::RectI renderRect(1, 1, tileWidth - 1, tileHeight - 1);

  frameScreen.OnSize(renderRect);

  /// commands from the previous sequence are ignored
  if (sequenceID < m_sequenceID)
    return;

  if (HasTile(rectInfo))
    return;

  shared_ptr<PaintEvent> paintEvent = make_shared_ptr(new PaintEvent(drawer, &env));

  my::Timer timer;

  shared_ptr<yg::gl::BaseTexture> tileTarget = m_resourceManager->renderTargets().Front(true);

  if (m_resourceManager->renderTargets().IsCancelled())
    return;

  drawer->screen()->setRenderTarget(tileTarget);

  shared_ptr<yg::InfoLayer> tileInfoLayer(new yg::InfoLayer());

  drawer->screen()->setInfoLayer(tileInfoLayer);

  drawer->beginFrame();
  drawer->clear(yg::Color(m_bgColor.r, m_bgColor.g, m_bgColor.b, 0));
  drawer->screen()->setClipRect(renderRect);
  drawer->clear(m_bgColor);

  frameScreen.SetFromRect(rectInfo.m_rect);

  m2::RectD selectionRect;

  double const inflationSize = 24 * drawer->VisualScale();
  //frameScreen.PtoG(m2::Inflate(m2::RectD(renderRect), inflationSize, inflationSize), selectionRect);
  frameScreen.PtoG(m2::RectD(renderRect), selectionRect);

  m_renderFn(
        paintEvent,
        frameScreen,
        selectionRect,
        rectInfo.m_drawScale
        );

  drawer->endFrame();
  drawer->screen()->resetInfoLayer();

  double duration = timer.ElapsedSeconds();

  if (env.IsCancelled())
    m_resourceManager->renderTargets().PushBack(tileTarget);
  else
    AddTile(rectInfo, Tile(tileTarget, tileInfoLayer, frameScreen, rectInfo, duration));
}

void TileRenderer::AddCommand(Tiler::RectInfo const & rectInfo, int sequenceID, core::CommandsQueue::Chain const & afterTileFns)
{
  m_sequenceID = sequenceID;

  core::CommandsQueue::Chain chain;
  chain.addCommand(bind(&TileRenderer::DrawTile, this, _1, rectInfo, sequenceID));
  chain.addCommand(afterTileFns);

  m_queue.AddCommand(chain);
}

TileCache & TileRenderer::GetTileCache()
{
  return m_tileCache;
}

void TileRenderer::WaitForEmptyAndFinished()
{
  m_queue.Join();
}

bool TileRenderer::HasTile(Tiler::RectInfo const & rectInfo)
{
  m_tileCache.readLock();
  bool res = m_tileCache.hasTile(rectInfo);
  m_tileCache.readUnlock();
  return res;
}

void TileRenderer::AddTile(Tiler::RectInfo const & rectInfo, Tile const & tile)
{
  m_tileCache.writeLock();
  if (m_tileCache.hasTile(rectInfo))
    m_tileCache.touchTile(rectInfo);
  else
    m_tileCache.addTile(rectInfo, TileCache::Entry(tile, m_resourceManager));
  m_tileCache.writeUnlock();
}

