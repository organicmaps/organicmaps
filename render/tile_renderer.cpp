#include "tile_renderer.hpp"
#include "window_handle.hpp"

#include "graphics/opengl/opengl.hpp"
#include "graphics/opengl/gl_render_context.hpp"
#include "graphics/opengl/base_texture.hpp"

#include "graphics/packets_queue.hpp"
#include "graphics/defines.hpp"

#include "indexer/scales.hpp"

#include "base/logging.hpp"
#include "base/condition.hpp"
#include "base/shared_buffer_manager.hpp"

#include "std/bind.hpp"

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

TileRenderer::TileSizeT TileRenderer::GetTileSizes() const
{
  graphics::ResourceManager::TexturePoolParams const & params =
      m_resourceManager->params().m_textureParams[graphics::ERenderTargetTexture];
  return make_pair(params.m_texWidth, params.m_texHeight);
}

TileRenderer::TileRenderer(
    size_t tileSize,
    unsigned executorsCount,
    graphics::Color const & bgColor,
    RenderPolicy::TRenderFn const & renderFn,
    shared_ptr<graphics::RenderContext> const & primaryRC,
    shared_ptr<graphics::ResourceManager> const & rm,
    double visualScale,
    graphics::PacketsQueue ** packetsQueues)
  : m_queue(executorsCount)
  , m_tileSize(tileSize)
  , m_renderFn(renderFn)
  , m_bgColor(bgColor)
  , m_sequenceID(0)
  , m_isPaused(false)
{
  m_resourceManager = rm;

  m_threadData.resize(m_queue.ExecutorsCount());

  LOG(LDEBUG, ("initializing ", m_queue.ExecutorsCount(), " rendering threads"));

  TileSizeT const tileSz = GetTileSizes();

  for (unsigned i = 0; i < m_threadData.size(); ++i)
  {
    if (!packetsQueues)
      m_threadData[i].m_renderContext.reset(primaryRC->createShared());

    GPUDrawer::Params params;

    params.m_screenParams.m_resourceManager = m_resourceManager;
    params.m_screenParams.m_frameBuffer = make_shared<graphics::gl::FrameBuffer>();

    params.m_screenParams.m_threadSlot = m_resourceManager->renderThreadSlot(i);
    params.m_visualScale = visualScale;
    if (packetsQueues != 0)
      params.m_screenParams.m_renderQueue = packetsQueues[i];
    params.m_screenParams.m_renderContext = m_threadData[i].m_renderContext;

    m_threadData[i].m_drawerParams = params;
    m_threadData[i].m_drawer = 0;
    m_threadData[i].m_threadSlot = params.m_screenParams.m_threadSlot;
    m_threadData[i].m_colorBuffer = make_shared<graphics::gl::RenderBuffer>(tileSz.first, tileSz.second, false,
                                                                            m_resourceManager->params().m_rgba4RenderBuffer);
    m_threadData[i].m_depthBuffer = make_shared<graphics::gl::RenderBuffer>(tileSz.first, tileSz.second, true);
  }

  m_queue.AddInitCommand(bind(&TileRenderer::InitializeThreadGL, this, _1));
  m_queue.AddFinCommand(bind(&TileRenderer::FinalizeThreadGL, this, _1));

  m_queue.Start();
}

TileRenderer::~TileRenderer()
{
}

void TileRenderer::Shutdown()
{
  LOG(LDEBUG, ("shutdown resources"));
  SetSequenceID(numeric_limits<int>::max());
  m_queue.CancelCommands();
  m_queue.Cancel();

  for (size_t i = 0; i < m_threadData.size(); ++i)
    if (m_threadData[i].m_drawer)
      delete m_threadData[i].m_drawer;
}

void TileRenderer::InitializeThreadGL(core::CommandsQueue::Environment const & env)
{
  LOG(LDEBUG, ("initializing TileRenderer", env.threadNum(), "on it's own thread"));

  ThreadData & threadData = m_threadData[env.threadNum()];

  TileSizeT const tileSz = GetTileSizes();

  if (threadData.m_renderContext)
  {
    threadData.m_renderContext->makeCurrent();
    threadData.m_renderContext->startThreadDrawing(threadData.m_threadSlot);
  }
  threadData.m_drawer = new GPUDrawer(threadData.m_drawerParams);
  threadData.m_drawer->OnSize(tileSz.first, tileSz.second);
  threadData.m_drawer->Screen()->setRenderTarget(threadData.m_colorBuffer);
  threadData.m_drawer->Screen()->setDepthBuffer(threadData.m_depthBuffer);
}

void TileRenderer::FinalizeThreadGL(core::CommandsQueue::Environment const & env)
{
  ThreadData & threadData = m_threadData[env.threadNum()];

  if (threadData.m_renderContext)
    threadData.m_renderContext->endThreadDrawing(threadData.m_threadSlot);
}

void TileRenderer::DrawTile(core::CommandsQueue::Environment const & env,
                           Tiler::RectInfo const & rectInfo,
                           int sequenceID)
{
#ifndef USE_DRAPE
  if (m_isPaused)
    return;

  /// commands from the previous sequence are ignored
  if (sequenceID < m_sequenceID)
    return;

  if (HasTile(rectInfo))
    return;

  ThreadData & threadData = m_threadData[env.threadNum()];

  graphics::PacketsQueue * glQueue = threadData.m_drawerParams.m_screenParams.m_renderQueue;

  GPUDrawer * drawer = threadData.m_drawer;

  ScreenBase frameScreen;

  TileSizeT const tileSz = GetTileSizes();

  m2::RectI renderRect(1, 1, tileSz.first - 1, tileSz.second - 1);

  frameScreen.OnSize(renderRect);

  shared_ptr<PaintEvent> paintEvent(new PaintEvent(drawer, &env));

  graphics::TTexturePool * texturePool = m_resourceManager->texturePool(graphics::ERenderTargetTexture);

  shared_ptr<graphics::gl::BaseTexture> tileTarget = texturePool->Reserve();

  if (texturePool->IsCancelled())
    return;

  shared_ptr<graphics::OverlayStorage> tileOverlay(new graphics::OverlayStorage(m2::RectD(renderRect)));

  graphics::Screen * pScreen = drawer->Screen();

  pScreen->setOverlay(tileOverlay);
  pScreen->beginFrame();
  pScreen->setClipRect(renderRect);
  pScreen->clear(m_bgColor);

  frameScreen.SetFromRect(m2::AnyRectD(rectInfo.m_rect));

  m_renderFn(paintEvent, frameScreen, m2::RectD(renderRect), rectInfo.m_tileScale);

  pScreen->endFrame();
  pScreen->resetOverlay();
  pScreen->copyFramebufferToImage(tileTarget);

  if (!env.IsCancelled())
  {
    if (glQueue)
      glQueue->completeCommands();
  }
  else
  {
    if (glQueue)
      glQueue->cancelCommands();
  }

  if (env.IsCancelled())
  {
    texturePool->Free(tileTarget);
  }
  else
  {
    AddActiveTile(Tile(tileTarget,
                 tileOverlay,
                 frameScreen,
                 rectInfo,
                 paintEvent->isEmptyDrawing(),
                 sequenceID));
  }
#endif //USE_DRAPE
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
      LOG(LDEBUG, ("resizing tileCache to", m_tileCache.CacheSize() + 1, "elements"));
      m_tileCache.Resize(m_tileCache.CacheSize() + 1);
    }

    m_tileCache.AddTile(rectInfo, TileCache::Entry(tile, m_resourceManager));
    m_tileSet.RemoveTile(rectInfo);
  }
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
    m_tileSet.AddTile(tile);
}

void TileRenderer::RemoveActiveTile(Tiler::RectInfo const & rectInfo, int sequenceID)
{
  TileStructuresLockGuard lock(m_tileCache, m_tileSet);

  if (m_tileSet.HasTile(rectInfo) && m_tileSet.GetTileSequenceID(rectInfo) <= sequenceID)
  {
    ASSERT(!m_tileCache.HasTile(rectInfo), ("Tile cannot be in tileSet and tileCache at the same time"));
    m_resourceManager->texturePool(graphics::ERenderTargetTexture)->Free(m_tileSet.GetTile(rectInfo).m_renderTarget);
    m_tileSet.RemoveTile(rectInfo);
  }
}

size_t TileRenderer::TileSize() const
{
  return m_tileSize;
}
