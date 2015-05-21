#include "coverage_generator.hpp"
#include "tile_renderer.hpp"
#include "tile_set.hpp"

#include "platform/settings.hpp"
#include "platform/platform.hpp"

#include "graphics/opengl/gl_render_context.hpp"
#include "graphics/display_list.hpp"

#include "base/logging.hpp"

#include "std/bind.hpp"


CoverageGenerator::CoverageGenerator(TileRenderer * tileRenderer,
                                     shared_ptr<WindowHandle> const & windowHandle,
                                     shared_ptr<graphics::RenderContext> const & primaryRC,
                                     shared_ptr<graphics::ResourceManager> const & rm,
                                     graphics::PacketsQueue * glQueue)
  : m_coverageInfo(tileRenderer)
  , m_queue(1)
  , m_windowHandle(windowHandle)
{
  shared_ptr<graphics::RenderContext> renderContext;
  if (!glQueue)
    renderContext.reset(primaryRC->createShared());

  m_queue.AddInitCommand(bind(&CoverageGenerator::InitializeThreadGL, this, renderContext, rm, glQueue));
  m_queue.AddFinCommand(bind(&CoverageGenerator::FinalizeThreadGL, this, renderContext, rm));

  m_queue.Start();

  Settings::Get("IsBenchmarking", m_benchmarkInfo.m_isBenchmarking);

  m_currentCoverage = new CachedCoverageInfo();
  m_backCoverage = new CachedCoverageInfo();
}

CoverageGenerator::~CoverageGenerator()
{
  LOG(LDEBUG, ("cancelling coverage thread"));
  ClearCoverage();
}

void CoverageGenerator::Shutdown()
{
  LOG(LDEBUG, ("shutdown resources"));
  m_stateInfo.SetSequenceID(numeric_limits<int>::max());
  m_queue.CancelCommands();
  m_queue.Cancel();
}

void CoverageGenerator::InitializeThreadGL(shared_ptr<graphics::RenderContext> context,
                                           shared_ptr<graphics::ResourceManager> resourceManager,
                                           graphics::PacketsQueue * glQueue)
{
  threads::MutexGuard g(m_stateInfo.m_mutex);

  LOG(LDEBUG, ("initializing CoverageGenerator on it's own thread."));

  if (context)
  {
    context->makeCurrent();
    context->startThreadDrawing(resourceManager->cacheThreadSlot());
  }

  graphics::Screen::Params params;

  params.m_resourceManager = resourceManager;
  params.m_renderQueue = glQueue;

  params.m_threadSlot = resourceManager->cacheThreadSlot();
  params.m_renderContext = context;
  params.m_storageType = graphics::EMediumStorage;
  params.m_textureType = graphics::EMediumTexture;

  m_cacheScreen.reset(new graphics::Screen(params));
}

void CoverageGenerator::FinalizeThreadGL(shared_ptr<graphics::RenderContext> context,
                                         shared_ptr<graphics::ResourceManager> resourceManager)
{
  if (context)
    context->endThreadDrawing(resourceManager->cacheThreadSlot());
}

void CoverageGenerator::Pause()
{
  m_stateInfo.Pause();
  m_queue.CancelCommands();
}

void CoverageGenerator::Resume()
{
  m_stateInfo.Resume();
}

void CoverageGenerator::InvalidateTiles(m2::AnyRectD const & r, int startScale)
{
  if (m_stateInfo.m_sequenceID == numeric_limits<int>::max())
    return;

  /// this automatically will skip the previous CoverScreen commands
  /// and MergeTiles commands from previously generated ScreenCoverages
  ++m_stateInfo.m_sequenceID;

  m_queue.AddCommand(bind(&CoverageGenerator::InvalidateTilesImpl, this, r, startScale));
}

void CoverageGenerator::CoverScreen(ScreenBase const & screen, bool doForce)
{
  if (screen == m_stateInfo.m_currentScreen && !doForce)
    return;

  if (m_stateInfo.m_sequenceID == numeric_limits<int>::max())
    return;

  m_stateInfo.m_currentScreen = screen;

  ++m_stateInfo.m_sequenceID;

  m_queue.AddCommand(bind(&CoverageGenerator::CoverScreenImpl, this, _1, screen, m_stateInfo.m_sequenceID));
}

void CoverageGenerator::MergeTile(Tiler::RectInfo const & rectInfo,
                                         int sequenceID)
{
  m_queue.AddCommand(bind(&CoverageGenerator::MergeTileImpl, this, _1, rectInfo, sequenceID));
}

void CoverageGenerator::FinishSequenceIfNeeded()
{
  m_queue.AddCommand(bind(&CoverageGenerator::BenchmarkInfo::TryFinishSequence, &m_benchmarkInfo));
}

void CoverageGenerator::Lock()
{
  m_stateInfo.m_mutex.Lock();
}

void CoverageGenerator::Unlock()
{
  m_stateInfo.m_mutex.Unlock();
}

void CoverageGenerator::Draw(graphics::Screen * s, ScreenBase const & screen)
{
  math::Matrix<double, 3, 3> m = m_currentCoverage->m_screen.PtoGMatrix() * screen.GtoPMatrix();

  ASSERT(m_currentCoverage, ());
  if (m_currentCoverage->m_mainElements)
    s->drawDisplayList(m_currentCoverage->m_mainElements, m);
  if (m_currentCoverage->m_sharpElements)
    s->drawDisplayList(m_currentCoverage->m_sharpElements, m);

  if (m_benchmarkInfo.m_isBenchmarking)
    FinishSequenceIfNeeded();
}

graphics::Overlay * CoverageGenerator::GetOverlay() const
{
  return m_coverageInfo.m_overlay;
}

bool CoverageGenerator::IsEmptyDrawing() const
{
  return (m_currentCoverage->m_renderLeafTilesCount <= 0) && m_currentCoverage->m_isEmptyDrawing;
}

bool CoverageGenerator::IsBackEmptyDrawing() const
{
  return (m_backCoverage->m_renderLeafTilesCount <= 0) && m_backCoverage->m_isEmptyDrawing;
}

bool CoverageGenerator::DoForceUpdate() const
{
  return m_stateInfo.m_needForceUpdate;
}

////////////////////////////////////////////////////
///           Benchmark support
////////////////////////////////////////////////////
int CoverageGenerator::InsertBenchmarkFence()
{
  return m_benchmarkInfo.InsertBenchmarkFence();
}

void CoverageGenerator::JoinBenchmarkFence(int fenceID)
{
  m_benchmarkInfo.JoinBenchmarkFence(fenceID);
}

////////////////////////////////////////////////////
///       On Coverage generator thread methods
////////////////////////////////////////////////////
void CoverageGenerator::CoverScreenImpl(core::CommandsQueue::Environment const & env,
                                        ScreenBase const & screen,
                                        int sequenceID)
{
  if (sequenceID < m_stateInfo.m_sequenceID)
    return;

  m_stateInfo.m_currentScreen = screen;
  m_backCoverage->m_screen = screen;
  m_coverageInfo.m_tiler.seed(screen, screen.GlobalRect().GlobalCenter(), m_coverageInfo.m_tileRenderer->TileSize());

  ComputeCoverTasks();

  bool const shouldSwap = !m_stateInfo.m_isPause && CacheCoverage(env);
  if (shouldSwap)
  {
    {
      threads::MutexGuard g(m_stateInfo.m_mutex);
      swap(m_currentCoverage, m_backCoverage);
    }

    m_backCoverage->ResetDL();
  }
  else
  {
    // we should skip all the following MergeTile commands
    ++m_stateInfo.m_sequenceID;
  }

  m_stateInfo.SetForceUpdate(!shouldSwap);

  m_windowHandle->invalidate();
}

void CoverageGenerator::MergeTileImpl(core::CommandsQueue::Environment const & env,
                                  Tiler::RectInfo const & rectInfo,
                                  int sequenceID)
{
  if (sequenceID < m_stateInfo.m_sequenceID)
  {
    m_coverageInfo.m_tileRenderer->RemoveActiveTile(rectInfo, sequenceID);
    return;
  }

  m_backCoverage->m_screen = m_stateInfo.m_currentScreen;
  m_backCoverage->m_isEmptyDrawing = m_currentCoverage->m_isEmptyDrawing;
  m_backCoverage->m_renderLeafTilesCount = m_currentCoverage->m_renderLeafTilesCount;
  MergeSingleTile(rectInfo);

  bool const shouldSwap = !m_stateInfo.m_isPause && CacheCoverage(env);
  if (shouldSwap)
  {
    {
      threads::MutexGuard g(m_stateInfo.m_mutex);
      swap(m_currentCoverage, m_backCoverage);
    }
    m_backCoverage->ResetDL();
  }

  m_benchmarkInfo.DecrementTileCount(sequenceID);

  m_stateInfo.SetForceUpdate(!shouldSwap);

  m_windowHandle->invalidate();
}

void CoverageGenerator::InvalidateTilesImpl(m2::AnyRectD const & r, int startScale)
{
  TileCache & tileCache = m_coverageInfo.m_tileRenderer->GetTileCache();
  tileCache.Lock();

  {
    threads::MutexGuard g(m_stateInfo.m_mutex);

    typedef buffer_vector<Tile const *, 8> vector8_t;
    vector8_t toRemove;

    for (CoverageInfo::TTileSet::const_iterator it = m_coverageInfo.m_tiles.begin();
         it != m_coverageInfo.m_tiles.end();
         ++it)
    {
      Tiler::RectInfo const & ri = (*it)->m_rectInfo;

      if (r.IsIntersect(m2::AnyRectD(ri.m_rect)) && (ri.m_tileScale >= startScale))
      {
        toRemove.push_back(*it);
        tileCache.UnlockTile(ri);
      }
    }

    for (vector8_t::const_iterator it = toRemove.begin();
         it != toRemove.end();
         ++it)
    {
      m_coverageInfo.m_tiles.erase(*it);
    }
  }

  /// here we should copy elements as we've delete some of them later
  set<Tiler::RectInfo> k = tileCache.Keys();

  for (set<Tiler::RectInfo>::const_iterator it = k.begin(); it != k.end(); ++it)
  {
    Tiler::RectInfo const & ri = *it;
    if ((ri.m_tileScale >= startScale) && r.IsIntersect(m2::AnyRectD(ri.m_rect)))
    {
      ASSERT(tileCache.LockCount(ri) == 0, ());
      tileCache.Remove(ri);
    }
  }

  tileCache.Unlock();

  MergeOverlay();
}

////////////////////////////////////////////////////////////
///             Support methods
////////////////////////////////////////////////////////////
void CoverageGenerator::ComputeCoverTasks()
{
  m_backCoverage->m_isEmptyDrawing = false;
  m_backCoverage->m_renderLeafTilesCount = 0;

  vector<Tiler::RectInfo> allRects;
  allRects.reserve(16);
  buffer_vector<Tiler::RectInfo, 8> newRects;
  m_coverageInfo.m_tiler.tiles(allRects, GetPlatform().PreCachingDepth());

  TileCache & tileCache = m_coverageInfo.m_tileRenderer->GetTileCache();
  tileCache.Lock();

  int const step = GetPlatform().PreCachingDepth() - 1;

  bool isEmptyDrawingBuf = true;
  CoverageInfo::TTileSet tiles;
  for (size_t i = 0; i < allRects.size(); ++i)
  {
    Tiler::RectInfo const & ri = allRects[i];

    if (!((ri.m_tileScale == m_coverageInfo.m_tiler.tileScale() - step) ||
          (ri.m_tileScale == m_coverageInfo.m_tiler.tileScale() )))
    {
      continue;
    }

    if (tileCache.HasTile(ri))
    {
      tileCache.TouchTile(ri);
      Tile const * tile = &tileCache.GetTile(ri);
      ASSERT(tiles.find(tile) == tiles.end(), ());

      if (m_coverageInfo.m_tiler.isLeaf(ri))
        isEmptyDrawingBuf &= tile->m_isEmptyDrawing;

      tiles.insert(tile);
    }
    else
    {
      newRects.push_back(ri);
      if (m_coverageInfo.m_tiler.isLeaf(ri))
        ++m_backCoverage->m_renderLeafTilesCount;
    }
  }

  m_backCoverage->m_isEmptyDrawing = isEmptyDrawingBuf;

  /// computing difference between current and previous coverage
  /// tiles, that aren't in current coverage are unlocked to allow their deletion from TileCache
  /// tiles, that are new to the current coverage are added into m_tiles and locked in TileCache

  size_t firstTileForAdd = 0;
  buffer_vector<const Tile *, 64> diff_tiles;
  diff_tiles.reserve(m_coverageInfo.m_tiles.size() + tiles.size());
#ifdef _MSC_VER
    vs_bug::
#endif
  set_difference(m_coverageInfo.m_tiles.begin(), m_coverageInfo.m_tiles.end(),
                 tiles.begin(), tiles.end(),
                 back_inserter(diff_tiles), tiles.key_comp());

  firstTileForAdd = diff_tiles.size();
#ifdef _MSC_VER
    vs_bug::
#endif
  set_difference(tiles.begin(), tiles.end(),
                 m_coverageInfo.m_tiles.begin(), m_coverageInfo.m_tiles.end(),
                 back_inserter(diff_tiles), tiles.key_comp());

  for (size_t i = 0; i < firstTileForAdd; ++i)
    tileCache.UnlockTile(diff_tiles[i]->m_rectInfo);

  for (size_t i = firstTileForAdd; i < diff_tiles.size(); ++i)
    tileCache.LockTile(diff_tiles[i]->m_rectInfo);

  tileCache.Unlock();

  m_coverageInfo.m_tiles = tiles;
  MergeOverlay();

  /// clearing all old commands
  m_coverageInfo.m_tileRenderer->ClearCommands();
  /// setting new sequenceID
  m_coverageInfo.m_tileRenderer->SetSequenceID(m_stateInfo.m_sequenceID);
  /// @todo After ClearCommands i think we have no commands to cancel.
  m_coverageInfo.m_tileRenderer->CancelCommands();

  m_benchmarkInfo.m_tilesCount = newRects.size();
  m_benchmarkInfo.m_benchmarkSequenceID = m_stateInfo.m_sequenceID;

  for (size_t i = 0; i < newRects.size(); ++i)
  {
    Tiler::RectInfo const & ri = newRects[i];

    core::CommandsQueue::Chain chain;

    chain.addCommand(bind(&CoverageGenerator::MergeTile,
                          this, ri, m_stateInfo.m_sequenceID));

    m_coverageInfo.m_tileRenderer->AddCommand(ri, m_stateInfo.m_sequenceID, chain);
  }
}

void CoverageGenerator::MergeOverlay()
{
  m_coverageInfo.m_overlay->lock();
  m_coverageInfo.m_overlay->clear();

  for (Tile const * tile : m_coverageInfo.m_tiles)
  {
    Tiler::RectInfo const & ri = tile->m_rectInfo;
    if (m_coverageInfo.m_tiler.isLeaf(ri))
      m_coverageInfo.m_overlay->merge(tile->m_overlay,
                                      tile->m_tileScreen.PtoGMatrix() * m_stateInfo.m_currentScreen.GtoPMatrix());
  }

  m_coverageInfo.m_overlay->unlock();
}

void CoverageGenerator::MergeSingleTile(Tiler::RectInfo const & rectInfo)
{
  m_coverageInfo.m_tileRenderer->CacheActiveTile(rectInfo);
  TileCache & tileCache = m_coverageInfo.m_tileRenderer->GetTileCache();
  tileCache.Lock();

  Tile const * tile = NULL;
  if (tileCache.HasTile(rectInfo))
  {
    tileCache.LockTile(rectInfo);
    tile = &tileCache.GetTile(rectInfo);
  }

  if (tile != NULL)
  {
    m_coverageInfo.m_tiles.insert(tile);

    if (m_coverageInfo.m_tiler.isLeaf(rectInfo))
    {
      m_backCoverage->m_isEmptyDrawing &= tile->m_isEmptyDrawing;
      m_backCoverage->m_renderLeafTilesCount--;
    }
  }

  tileCache.Unlock();

  if (tile != NULL && m_coverageInfo.m_tiler.isLeaf(rectInfo))
  {
    m_coverageInfo.m_overlay->lock();
    m_coverageInfo.m_overlay->merge(tile->m_overlay,
                                    tile->m_tileScreen.PtoGMatrix() * m_stateInfo.m_currentScreen.GtoPMatrix());
    m_coverageInfo.m_overlay->unlock();
  }
}

namespace
{
  bool SharpnessComparator(shared_ptr<graphics::OverlayElement> const & e1,
                           shared_ptr<graphics::OverlayElement> const & e2)
  {
    return e1->hasSharpGeometry() && (!e2->hasSharpGeometry());
  }
}

bool CoverageGenerator::CacheCoverage(core::CommandsQueue::Environment const & env)
{
  if (m_cacheScreen->isCancelled())
    return false;

  ASSERT(m_backCoverage->m_mainElements == nullptr, ());
  ASSERT(m_backCoverage->m_sharpElements == nullptr, ());

  m_backCoverage->m_mainElements = m_cacheScreen->createDisplayList();
  m_backCoverage->m_sharpElements = m_cacheScreen->createDisplayList();

  m_cacheScreen->setEnvironment(&env);

  m_cacheScreen->beginFrame();
  m_cacheScreen->setDisplayList(m_backCoverage->m_mainElements);

  vector<graphics::BlitInfo> infos;
  infos.reserve(m_coverageInfo.m_tiles.size());

  for (Tile const * tile : m_coverageInfo.m_tiles)
  {
    size_t tileWidth = tile->m_renderTarget->width();
    size_t tileHeight = tile->m_renderTarget->height();

    graphics::BlitInfo bi;

    bi.m_matrix = tile->m_tileScreen.PtoGMatrix() * m_stateInfo.m_currentScreen.GtoPMatrix();
    bi.m_srcRect = m2::RectI(0, 0, tileWidth - 2, tileHeight - 2);
    bi.m_texRect = m2::RectU(1, 1, tileWidth - 1, tileHeight - 1);
    bi.m_srcSurface = tile->m_renderTarget;
    bi.m_depth = tile->m_rectInfo.m_tileScale * 100;

    infos.push_back(bi);
  }

  if (!infos.empty())
    m_cacheScreen->blit(infos.data(), infos.size(), true);

  m_cacheScreen->clear(graphics::Color(), false);

  math::Matrix<double, 3, 3> idM = math::Identity<double, 3>();

  m_coverageInfo.m_overlay->lock();

  vector<shared_ptr<graphics::OverlayElement> > overlayElements;
  overlayElements.reserve(m_coverageInfo.m_overlay->getElementsCount());
  m_coverageInfo.m_overlay->forEach(MakeBackInsertFunctor(overlayElements));
  sort(overlayElements.begin(), overlayElements.end(), SharpnessComparator);

  unsigned currentElement = 0;

  for (; currentElement < overlayElements.size(); ++currentElement)
  {
    shared_ptr<graphics::OverlayElement> const & elem = overlayElements[currentElement];
    if (elem->hasSharpGeometry())
      break;

    elem->draw(m_cacheScreen.get(), idM);
  }

  m_cacheScreen->applySharpStates();
  m_cacheScreen->setDisplayList(m_backCoverage->m_sharpElements);

  for (; currentElement < overlayElements.size(); ++currentElement)
    overlayElements[currentElement]->draw(m_cacheScreen.get(), idM);

  m_coverageInfo.m_overlay->unlock();

  m_cacheScreen->setDisplayList(0);
  m_cacheScreen->applyStates();

  m_cacheScreen->endFrame();

  /// completing commands that was immediately executed
  /// while recording of displayList(for example UnlockStorage)

  m_cacheScreen->completeCommands();

  m_cacheScreen->setEnvironment(0);

  return true;
}

void CoverageGenerator::ClearCoverage()
{
  {
    m_coverageInfo.m_overlay->lock();
    m_coverageInfo.m_overlay->clear();
    m_coverageInfo.m_overlay->unlock();
  }

  TileCache & tileCache = m_coverageInfo.m_tileRenderer->GetTileCache();

  tileCache.Lock();

  for (CoverageInfo::TTileSet::const_iterator it = m_coverageInfo.m_tiles.begin();
       it != m_coverageInfo.m_tiles.end();
       ++it)
  {
    Tiler::RectInfo const & ri = (*it)->m_rectInfo;
    tileCache.UnlockTile(ri);
  }

  tileCache.Unlock();

  m_coverageInfo.m_tiles.clear();

  delete m_currentCoverage;
  m_currentCoverage = 0;
  delete m_backCoverage;
  m_backCoverage = 0;
}

////////////////////////////////////////////////////////////
///             BenchmarkInfo
////////////////////////////////////////////////////////////

CoverageGenerator::BenchmarkInfo::BenchmarkInfo()
  : m_benchmarkSequenceID(numeric_limits<int>::max())
  , m_tilesCount(-1)
  , m_fenceManager(2)
  , m_currentFenceID(-1)
  , m_isBenchmarking(false)
{
}

void CoverageGenerator::BenchmarkInfo::DecrementTileCount(int sequenceID)
{
  if (!m_isBenchmarking)
    return;

  if (sequenceID < m_benchmarkSequenceID)
  {
    m_tilesCount = -1;
    return;
  }
  m_tilesCount -= 1;
}

int CoverageGenerator::BenchmarkInfo::InsertBenchmarkFence()
{
  ASSERT(m_isBenchmarking, ("Only for benchmark mode"));
  m_currentFenceID = m_fenceManager.insertFence();
  return m_currentFenceID;
}

void CoverageGenerator::BenchmarkInfo::JoinBenchmarkFence(int fenceID)
{
  ASSERT(m_isBenchmarking, ("Only for benchmark mode"));
  CHECK(fenceID == m_currentFenceID, ("InsertBenchmarkFence without corresponding SignalBenchmarkFence detected"));
  m_fenceManager.joinFence(fenceID);
}

void CoverageGenerator::BenchmarkInfo::SignalBenchmarkFence()
{
  ASSERT(m_isBenchmarking, ("Only for benchmark mode"));
  if (m_currentFenceID != -1)
    m_fenceManager.signalFence(m_currentFenceID);
}

void CoverageGenerator::BenchmarkInfo::TryFinishSequence()
{
  if (m_tilesCount == 0)
  {
    m_tilesCount = -1;
    SignalBenchmarkFence();
  }
}

////////////////////////////////////////////////////////////
///             StateInfo
////////////////////////////////////////////////////////////
CoverageGenerator::StateInfo::StateInfo()
  : m_isPause(false)
  , m_needForceUpdate(false)
  , m_sequenceID(0)
{
}

void CoverageGenerator::StateInfo::SetSequenceID(int sequenceID)
{
  m_sequenceID = sequenceID;
}

void CoverageGenerator::StateInfo::SetForceUpdate(bool needForceUpdate)
{
  m_needForceUpdate = needForceUpdate;
}

void CoverageGenerator::StateInfo::Pause()
{
  m_isPause = true;
}

void CoverageGenerator::StateInfo::Resume()
{
  m_isPause = false;
  m_needForceUpdate = true;
}

////////////////////////////////////////////////////////////
///             CoverageGenerator
////////////////////////////////////////////////////////////
CoverageGenerator::CoverageInfo::CoverageInfo(TileRenderer *tileRenderer)
  : m_tileRenderer(tileRenderer)
  , m_overlay(new graphics::Overlay())
{
}

CoverageGenerator::CoverageInfo::~CoverageInfo()
{
  delete m_overlay;
}

CoverageGenerator::CachedCoverageInfo::CachedCoverageInfo()
  : m_mainElements(nullptr)
  , m_sharpElements(nullptr)
  , m_renderLeafTilesCount(0)
  , m_isEmptyDrawing(false)
{
}

CoverageGenerator::CachedCoverageInfo::~CachedCoverageInfo()
{
  delete m_mainElements;
  delete m_sharpElements;
}

void CoverageGenerator::CachedCoverageInfo::ResetDL()
{
  delete m_mainElements;
  m_mainElements = nullptr;
  delete m_sharpElements;
  m_sharpElements = nullptr;
}
