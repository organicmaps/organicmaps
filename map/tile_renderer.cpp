#include "../base/SRC_FIRST.hpp"

#include "tile_renderer.hpp"
#include "window_handle.hpp"

#include "../graphics/opengl/opengl.hpp"
#include "../graphics/opengl/gl_render_context.hpp"
#include "../graphics/opengl/base_texture.hpp"

#include "../graphics/packets_queue.hpp"
#include "../graphics/defines.hpp"

#include "../std/bind.hpp"

#include "../indexer/scales.hpp"

#include "../base/logging.hpp"
#include "../base/condition.hpp"
#include "../base/shared_buffer_manager.hpp"

namespace
{
  class TileStructuresLockGuard
  {
  public:
    TileStructuresLockGuard(TileCache & tileCache, TileSet & tileSet)
      : m_tileCache(tileCache), m_tileSet(tileSet)
    {
      m_tileSet.Lock();
      m_tileCache.Lock();
    }

    ~TileStructuresLockGuard()
    {
      m_tileCache.Unlock();
      m_tileSet.Unlock();
    }

  private:
    TileCache & m_tileCache;
    TileSet & m_tileSet;
  };
}

TileRenderer::TileRenderer(
    size_t tileSize,
    string const & skinName,
    graphics::EDensity density,
    unsigned executorsCount,
    graphics::Color const & bgColor,
    RenderPolicy::TRenderFn const & renderFn,
    shared_ptr<graphics::RenderContext> const & primaryRC,
    shared_ptr<graphics::ResourceManager> const & rm,
    double visualScale,
    graphics::PacketsQueue ** packetsQueues
  ) : m_queue(executorsCount),
      m_tileSize(tileSize),
      m_renderFn(renderFn),
      m_skinName(skinName),
      m_density(density),
      m_bgColor(bgColor),
      m_sequenceID(0),
      m_isExiting(false),
      m_isPaused(false)
{
  m_resourceManager = rm;
  m_primaryContext = primaryRC;

  m_threadData.resize(m_queue.ExecutorsCount());

  LOG(LINFO, ("initializing ", m_queue.ExecutorsCount(), " rendering threads"));

  int tileWidth = m_resourceManager->params().m_textureParams[graphics::ERenderTargetTexture].m_texWidth;
  int tileHeight = m_resourceManager->params().m_textureParams[graphics::ERenderTargetTexture].m_texHeight;

  for (unsigned i = 0; i < m_threadData.size(); ++i)
  {
    if (!packetsQueues)
      m_threadData[i].m_renderContext.reset(m_primaryContext->createShared());

    Drawer::Params params;

    params.m_resourceManager = m_resourceManager;
    params.m_frameBuffer = make_shared_ptr(new graphics::gl::FrameBuffer());

    params.m_threadSlot = m_resourceManager->renderThreadSlot(i);
    params.m_visualScale = visualScale;
    if (packetsQueues != 0)
      params.m_renderQueue = packetsQueues[i];
    params.m_doUnbindRT = false;
    params.m_isSynchronized = false;
    params.m_skinName = m_skinName;
    params.m_density = m_density;
    params.m_renderContext = m_threadData[i].m_renderContext;
  /*  params.m_isDebugging = true;
    params.m_drawPathes = false ;
    params.m_drawAreas = false;
    params.m_drawTexts = false; */

    m_threadData[i].m_drawerParams = params;
    m_threadData[i].m_drawer = 0;
    m_threadData[i].m_threadSlot = params.m_threadSlot;

    m_threadData[i].m_dummyRT = m_resourceManager->createRenderTarget(2, 2);
    m_threadData[i].m_depthBuffer = make_shared_ptr(new graphics::gl::RenderBuffer(tileWidth, tileHeight, true));
  }

  m_queue.AddInitCommand(bind(&TileRenderer::InitializeThreadGL, this, _1));
  m_queue.AddFinCommand(bind(&TileRenderer::FinalizeThreadGL, this, _1));

  m_queue.Start();
}

TileRenderer::~TileRenderer()
{
  m_isExiting = true;

  //LOG(LDEBUG, ("UVRLOG : Cancel from ~TileRenderer"));
  m_queue.Cancel();

  for (size_t i = 0; i < m_threadData.size(); ++i)
    if (m_threadData[i].m_drawer)
      delete m_threadData[i].m_drawer;
}

void TileRenderer::InitializeThreadGL(core::CommandsQueue::Environment const & env)
{
  LOG(LINFO, ("initializing TileRenderer", env.threadNum(), "on it's own thread"));

  ThreadData & threadData = m_threadData[env.threadNum()];

  int tileWidth = m_resourceManager->params().m_textureParams[graphics::ERenderTargetTexture].m_texWidth;
  int tileHeight = m_resourceManager->params().m_textureParams[graphics::ERenderTargetTexture].m_texHeight;

  if (threadData.m_renderContext)
  {
    threadData.m_renderContext->makeCurrent();
    threadData.m_renderContext->startThreadDrawing(threadData.m_threadSlot);
  }
  threadData.m_drawer = new Drawer(threadData.m_drawerParams);
  threadData.m_drawer->onSize(tileWidth, tileHeight);
  threadData.m_drawer->screen()->setDepthBuffer(threadData.m_depthBuffer);
}

void TileRenderer::FinalizeThreadGL(core::CommandsQueue::Environment const & env)
{
  ThreadData & threadData = m_threadData[env.threadNum()];

  if (threadData.m_renderContext)
    threadData.m_renderContext->endThreadDrawing(threadData.m_threadSlot);
}

void TileRenderer::ReadPixels(graphics::PacketsQueue * glQueue, core::CommandsQueue::Environment const & env)
{
  ThreadData & threadData = m_threadData[env.threadNum()];

  Drawer * drawer = threadData.m_drawer;

  if (glQueue)
  {
    glQueue->processFn(bind(&TileRenderer::ReadPixels, this, (graphics::PacketsQueue*)0, ref(env)), true);
    return;
  }

  if (!env.isCancelled())
  {
    unsigned tileWidth = m_resourceManager->params().m_textureParams[graphics::ERenderTargetTexture].m_texWidth;
    unsigned tileHeight = m_resourceManager->params().m_textureParams[graphics::ERenderTargetTexture].m_texHeight;

    shared_ptr<vector<unsigned char> > buf = SharedBufferManager::instance().reserveSharedBuffer(tileWidth * tileHeight * 4);
    drawer->screen()->readPixels(m2::RectU(0, 0, tileWidth, tileHeight), &(buf->at(0)), true);
    SharedBufferManager::instance().freeSharedBuffer(tileWidth * tileHeight * 4, buf);
  }
}

void TileRenderer::DrawTile(core::CommandsQueue::Environment const & env,
                           Tiler::RectInfo const & rectInfo,
                           int sequenceID)
{
  if (m_isPaused)
    return;

  /// commands from the previous sequence are ignored
  if (sequenceID < m_sequenceID)
  {
    //LOG(LDEBUG, ("UVRLOG : tile not rendered. SequenceID=", sequenceID, " m_SequenceID", m_sequenceID));
    return;
  }

  //LOG(LDEBUG, ("UVRLOG : start render tile m_SequenceID=", m_sequenceID));

  if (HasTile(rectInfo))
    return;

  ThreadData & threadData = m_threadData[env.threadNum()];

  graphics::PacketsQueue * glQueue = threadData.m_drawerParams.m_renderQueue;

  Drawer * drawer = threadData.m_drawer;

  ScreenBase frameScreen;

  unsigned tileWidth = m_resourceManager->params().m_textureParams[graphics::ERenderTargetTexture].m_texWidth;
  unsigned tileHeight = m_resourceManager->params().m_textureParams[graphics::ERenderTargetTexture].m_texHeight;

  m2::RectI renderRect(1, 1, tileWidth - 1, tileHeight - 1);

  frameScreen.OnSize(renderRect);

  shared_ptr<PaintEvent> paintEvent = make_shared_ptr(new PaintEvent(drawer, &env));

  graphics::TTexturePool * texturePool = m_resourceManager->texturePool(graphics::ERenderTargetTexture);

  shared_ptr<graphics::gl::BaseTexture> tileTarget = texturePool->Reserve();

  if (texturePool->IsCancelled())
    return;

  drawer->screen()->setRenderTarget(tileTarget);

  shared_ptr<graphics::Overlay> tileOverlay(new graphics::Overlay());
  tileOverlay->setCouldOverlap(true);

  drawer->screen()->setOverlay(tileOverlay);

  /// ensuring, that the render target is not bound as a texture

  threadData.m_dummyRT->makeCurrent(glQueue);

  drawer->beginFrame();
  drawer->clear(graphics::Color(m_bgColor.r, m_bgColor.g, m_bgColor.b, 0));
  drawer->screen()->setClipRect(renderRect);
  drawer->clear(m_bgColor);

/*  drawer->clear(graphics::Color(rand() % 32 + 128, rand() % 64 + 128, rand() % 32 + 128, 255));

  std::stringstream out;
  out << rectInfo.m_y << ", " << rectInfo.m_x << ", " << rectInfo.m_tileScale;

  drawer->screen()->drawText(graphics::FontDesc(12, graphics::Color(0, 0, 0, 255), true),
                             renderRect.Center(),
                             graphics::EPosCenter,
                             out.str(),
                             0,
                             false);*/
  frameScreen.SetFromRect(m2::AnyRectD(rectInfo.m_rect));

  m2::RectD selectRect;
  m2::RectD clipRect;

  double const inflationSize = 24 * drawer->VisualScale();
  frameScreen.PtoG(m2::Inflate(m2::RectD(renderRect), inflationSize, inflationSize), clipRect);
  frameScreen.PtoG(m2::RectD(renderRect), selectRect);

  // adjusting tileScale to look the same across devices with different tileWidth and visualScale values
  int styleTileScale = max((rectInfo.m_tileScale + log(tileWidth / 256.0 / drawer->VisualScale()) / log(2.0)), 1.0);
  m_renderFn(
        paintEvent,
        frameScreen,
        selectRect,
        clipRect,
        styleTileScale,
        styleTileScale <= scales::GetUpperScale()
        );

  drawer->endFrame();

  drawer->screen()->resetOverlay();

  /// filter out the overlay elements that are out of the bound rect for the tile
  if (!env.isCancelled())
    tileOverlay->clip(renderRect);

  drawer->screen()->finish();

  if (m_resourceManager->useReadPixelsToSynchronize())
    ReadPixels(glQueue, env);

  drawer->screen()->unbindRenderTarget();

  if (!env.isCancelled())
  {
    if (glQueue)
      glQueue->completeCommands();

    AddActiveTile(Tile(tileTarget,
                 tileOverlay,
                 frameScreen,
                 rectInfo,
                 0,
                 paintEvent->isEmptyDrawing(),
                 sequenceID));
  }
  else
  {
    if (!m_isExiting)
    {
      if (glQueue)
        glQueue->cancelCommands();

      texturePool->Free(tileTarget);
    }
  }
}

void TileRenderer::AddCommand(Tiler::RectInfo const & rectInfo, int sequenceID, core::CommandsQueue::Chain const & afterTileFns)
{
  SetSequenceID(sequenceID);

  core::CommandsQueue::Chain chain;
  chain.addCommand(bind(&TileRenderer::DrawTile, this, _1, rectInfo, sequenceID));
  chain.addCommand(afterTileFns);

  m_queue.AddCommand(chain);
}

void TileRenderer::CancelCommands()
{
  m_queue.CancelCommands();
}

void TileRenderer::ClearCommands()
{
  m_queue.Clear();
}

void TileRenderer::SetSequenceID(int sequenceID)
{
  m_sequenceID = sequenceID;
}

TileCache & TileRenderer::GetTileCache()
{
  return m_tileCache;
}

void TileRenderer::CacheActiveTile(Tiler::RectInfo const & rectInfo)
{
  TileStructuresLockGuard guard(m_tileCache, m_tileSet);
  if (m_tileSet.HasTile(rectInfo))
  {
    ASSERT(!m_tileCache.HasTile(rectInfo), (""));
    Tile tile = m_tileSet.GetTile(rectInfo);

    if (m_tileCache.CanFit() == 0)
    {
      LOG(LINFO, ("resizing tileCache to", m_tileCache.CacheSize() + 1, "elements"));
      m_tileCache.Resize(m_tileCache.CacheSize() + 1);
    }

    m_tileCache.AddTile(rectInfo, TileCache::Entry(tile, m_resourceManager));
    m_tileSet.RemoveTile(rectInfo);
  }
}

void TileRenderer::WaitForEmptyAndFinished()
{
  m_queue.Join();
}

bool TileRenderer::HasTile(Tiler::RectInfo const & rectInfo)
{
  TileStructuresLockGuard guard(m_tileCache, m_tileSet);

  if (m_tileSet.HasTile(rectInfo))
  {
    m_tileSet.SetTileSequenceID(rectInfo, m_sequenceID);
    return true;
  }
  TileCache & tileCache = GetTileCache();
  if (tileCache.HasTile(rectInfo))
    return true;

  return false;
}

void TileRenderer::SetIsPaused(bool flag)
{
  m_isPaused = flag;
}

void TileRenderer::AddActiveTile(Tile const & tile)
{
  TileStructuresLockGuard lock(m_tileCache, m_tileSet);

  Tiler::RectInfo const & key = tile.m_rectInfo;

  if (m_tileSet.HasTile(key) || m_tileCache.HasTile(key))
    m_resourceManager->texturePool(graphics::ERenderTargetTexture)->Free(tile.m_renderTarget);
  else
  {
    //LOG(LDEBUG, ("UVRLOG : Add tile to set s=", key.m_tileScale, " x=", key.m_x, " y=", key.m_y, " m_SequenceID=", m_sequenceID));
    m_tileSet.AddTile(tile);
  }
}

void TileRenderer::RemoveActiveTile(Tiler::RectInfo const & rectInfo, int sequenceID)
{
  TileStructuresLockGuard lock(m_tileCache, m_tileSet);

  if (m_tileSet.HasTile(rectInfo) && m_tileSet.GetTileSequenceID(rectInfo) <= sequenceID)
  {
    ASSERT(!m_tileCache.HasTile(rectInfo), ("Tile cannot be in tileSet and tileCache at the same time"));
    //LOG(LDEBUG, ("UVRLOG : Remove tile from set s=", rectInfo.m_tileScale, " x=", rectInfo.m_x, " y=", rectInfo.m_y, " m_SequenceID=", m_sequenceID));
    m_resourceManager->texturePool(graphics::ERenderTargetTexture)->Free(m_tileSet.GetTile(rectInfo).m_renderTarget);
    m_tileSet.RemoveTile(rectInfo);
  }
}

size_t TileRenderer::TileSize() const
{
  return m_tileSize;
}
