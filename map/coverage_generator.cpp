#include "../base/SRC_FIRST.hpp"
#include "../platform/settings.hpp"

#include "coverage_generator.hpp"
#include "screen_coverage.hpp"
#include "tile_renderer.hpp"
#include "tile_set.hpp"

#include "../graphics/opengl/gl_render_context.hpp"

#include "../base/logging.hpp"

#include "../std/bind.hpp"

bool g_coverageGeneratorDestroyed = false;

CoverageGenerator::CoverageGenerator(
    string const & skinName,
    graphics::EDensity density,
    TileRenderer * tileRenderer,
    shared_ptr<WindowHandle> const & windowHandle,
    shared_ptr<graphics::RenderContext> const & primaryRC,
    shared_ptr<graphics::ResourceManager> const & rm,
    graphics::PacketsQueue * glQueue,
    RenderPolicy::TCountryIndexFn const & countryIndexFn)
  : m_queue(1),
    m_tileRenderer(tileRenderer),
    m_workCoverage(0),
    m_currentCoverage(0),
    m_sequenceID(0),
    m_windowHandle(windowHandle),
    m_countryIndexFn(countryIndexFn),
    m_glQueue(glQueue),
    m_skinName(skinName),
    m_density(density),
    m_fenceManager(2),
    m_currentFenceID(-1),
    m_doForceUpdate(false),
    m_isPaused(false),
    m_isBenchmarking(false)
{
  g_coverageGeneratorDestroyed = false;

  m_resourceManager = rm;

  if (!m_glQueue)
    m_renderContext.reset(primaryRC->createShared());

  m_queue.AddInitCommand(bind(&CoverageGenerator::InitializeThreadGL, this));
  m_queue.AddFinCommand(bind(&CoverageGenerator::FinalizeThreadGL, this));

  m_queue.Start();

  Settings::Get("IsBenchmarking", m_isBenchmarking);
}

ScreenCoverage * CoverageGenerator::CreateCoverage()
{
  graphics::Screen::Params params;

  params.m_resourceManager = m_resourceManager;
  params.m_renderQueue = m_glQueue;

  params.m_doUnbindRT = false;
  params.m_isSynchronized = false;
  params.m_threadSlot = m_resourceManager->cacheThreadSlot();
  params.m_renderContext = m_renderContext;
  params.m_storageType = graphics::EMediumStorage;
  params.m_textureType = graphics::EMediumTexture;
  params.m_skinName = m_skinName;
  params.m_density = m_density;

  shared_ptr<graphics::Screen> screen(new graphics::Screen(params));

  ScreenCoverage * screenCoverage = new ScreenCoverage(m_tileRenderer, this, screen);
  screenCoverage->SetBenchmarkingFlag(m_isBenchmarking);

  return screenCoverage;
}

void CoverageGenerator::InitializeThreadGL()
{
  threads::MutexGuard g(m_mutex);

  LOG(LINFO, ("initializing CoverageGenerator on it's own thread."));

  if (m_renderContext)
  {
    m_renderContext->makeCurrent();
    m_renderContext->startThreadDrawing(m_resourceManager->cacheThreadSlot());
  }

  m_workCoverage = CreateCoverage();
  m_currentCoverage = CreateCoverage();
}

void CoverageGenerator::FinalizeThreadGL()
{
  if (m_renderContext)
    m_renderContext->endThreadDrawing(m_resourceManager->cacheThreadSlot());
}

CoverageGenerator::~CoverageGenerator()
{
  LOG(LINFO, ("cancelling coverage thread"));
  Cancel();

  LOG(LINFO, ("deleting workCoverage"));
  delete m_workCoverage;
  m_workCoverage = 0;

  LOG(LINFO, ("deleting currentCoverage"));
  delete m_currentCoverage;
  m_currentCoverage = 0;

  g_coverageGeneratorDestroyed = true;
}

void CoverageGenerator::Cancel()
{
  //LOG(LDEBUG, ("UVRLOG : CoverageGenerator::Cancel"));
  m_queue.Cancel();
}

void CoverageGenerator::InvalidateTilesImpl(m2::AnyRectD const & r, int startScale)
{
  {
    threads::MutexGuard g(m_mutex);
    m_currentCoverage->RemoveTiles(r, startScale);
  }

  TileCache & tileCache = m_tileRenderer->GetTileCache();

  tileCache.Lock();

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
}

void CoverageGenerator::InvalidateTiles(m2::AnyRectD const & r, int startScale)
{
  if (m_sequenceID == numeric_limits<int>::max())
    return;

  /// this automatically will skip the previous CoverScreen commands
  /// and MergeTiles commands from previously generated ScreenCoverages
  ++m_sequenceID;

  m_queue.AddCommand(bind(&CoverageGenerator::InvalidateTilesImpl, this, r, startScale));
}

void CoverageGenerator::AddCoverScreenTask(ScreenBase const & screen, bool doForce)
{
  if ((screen == m_currentScreen) && (!doForce))
    return;

  if (m_sequenceID == numeric_limits<int>::max())
    return;

  m_currentScreen = screen;

  ++m_sequenceID;

  m_queue.AddCommand(bind(&CoverageGenerator::CoverScreen, this, _1, screen, m_sequenceID));
}

int CoverageGenerator::InsertBenchmarkFence()
{
  ASSERT(m_isBenchmarking, ("Only in benchmarking mode!"));
  m_currentFenceID = m_fenceManager.insertFence();
  return m_currentFenceID;
}

void CoverageGenerator::JoinBenchmarkFence(int fenceID)
{
  ASSERT(m_isBenchmarking, ("Only in benchmarking mode!"));
  CHECK(fenceID == m_currentFenceID, ("InsertBenchmarkFence without corresponding SignalBenchmarkFence detected"));
  m_fenceManager.joinFence(fenceID);
}

void CoverageGenerator::SignalBenchmarkFence()
{
  ASSERT(m_isBenchmarking, ("Only in benchmarking mode!"));
  if (m_currentFenceID != -1)
    m_fenceManager.signalFence(m_currentFenceID);
}

void CoverageGenerator::CoverScreen(core::CommandsQueue::Environment const & env,
                                    ScreenBase const & screen,
                                    int sequenceID)
{
  if (sequenceID < m_sequenceID)
    return;

  m_currentCoverage->CopyInto(*m_workCoverage, false);

  m_workCoverage->SetSequenceID(sequenceID);
  m_workCoverage->SetScreen(screen);

  if (!m_workCoverage->IsPartialCoverage() && m_workCoverage->IsEmptyDrawingCoverage())
  {
    m_workCoverage->ResetEmptyModelAtCoverageCenter();
    AddCheckEmptyModelTask(sequenceID);
  }

  bool shouldSwap = !m_isPaused && m_workCoverage->Cache(env);

  if (shouldSwap)
  {
    threads::MutexGuard g(m_mutex);
    swap(m_currentCoverage, m_workCoverage);
  }
  else
  {
    /// we should skip all the following MergeTile commands
    ++m_sequenceID;
  }

  m_doForceUpdate = !shouldSwap;

  m_workCoverage->Clear();

  m_windowHandle->invalidate();
}

void CoverageGenerator::AddMergeTileTask(Tiler::RectInfo const & rectInfo,
                                         int sequenceID)
{
  if (g_coverageGeneratorDestroyed)
    return;

  m_queue.AddCommand(bind(&CoverageGenerator::MergeTile, this, _1, rectInfo, sequenceID));
}

void CoverageGenerator::MergeTile(core::CommandsQueue::Environment const & env,
                                  Tiler::RectInfo const & rectInfo,
                                  int sequenceID)
{
  if (sequenceID < m_sequenceID)
  {
    //LOG(LDEBUG, ("UVRLOG : MergeTile fail. s=", rectInfo.m_tileScale, " x=", rectInfo.m_x, " y=", rectInfo.m_y, " SequenceID=", sequenceID, " m_SequenceID=", m_sequenceID));
    m_tileRenderer->RemoveActiveTile(rectInfo, sequenceID);
    return;
  }

  //LOG(LDEBUG, ("UVRLOG : MergeTile s=", rectInfo.m_tileScale, " x=", rectInfo.m_x, " y=", rectInfo.m_y, " SequenceID=", sequenceID, " m_SequenceID=", m_sequenceID));
  m_currentCoverage->CopyInto(*m_workCoverage, true);
  m_workCoverage->SetSequenceID(sequenceID);
  m_workCoverage->Merge(rectInfo);

  if (!m_workCoverage->IsPartialCoverage() && m_workCoverage->IsEmptyDrawingCoverage())
  {
    m_workCoverage->ResetEmptyModelAtCoverageCenter();
    AddCheckEmptyModelTask(sequenceID);
  }

  bool shouldSwap = !m_isPaused && m_workCoverage->Cache(env);

  if (shouldSwap)
  {
    threads::MutexGuard g(m_mutex);
    swap(m_currentCoverage, m_workCoverage);
  }

  m_doForceUpdate = !shouldSwap;

  m_workCoverage->Clear();

  if (!m_isBenchmarking)
    m_windowHandle->invalidate();
}

void CoverageGenerator::AddCheckEmptyModelTask(int sequenceID)
{
  m_queue.AddCommand(bind(&CoverageGenerator::CheckEmptyModel, this, sequenceID));
}

void CoverageGenerator::CheckEmptyModel(int sequenceID)
{
  if (sequenceID < m_sequenceID)
    return;

  m_currentCoverage->CheckEmptyModelAtCoverageCenter();

  m_windowHandle->invalidate();
}

void CoverageGenerator::AddFinishSequenceTaskIfNeeded()
{
  if (g_coverageGeneratorDestroyed)
    return;

  if (m_benchmarkBarrier.m_tilesCount == 0)
  {
    // this instruction based on idea that up to this point all tiles is rendered, all threads
    // expect gui thread in wait state, and no one can modify m_tilesCount expect gui thread.
    // If banchmarking engine change logic, may be m_tilesCount need to be guard by some sync primitive
    m_benchmarkBarrier.m_tilesCount = -1;
    m_queue.AddCommand(bind(&CoverageGenerator::FinishSequence, this));
  }
}

void CoverageGenerator::FinishSequence()
{
  SignalBenchmarkFence();
}

void CoverageGenerator::AddDecrementTileCountTask(int sequenceID)
{
  if (g_coverageGeneratorDestroyed)
    return;

  m_queue.AddCommand(bind(&CoverageGenerator::DecrementTileCounter, this, sequenceID));
}

void CoverageGenerator::DecrementTileCounter(int sequenceID)
{
  ASSERT(m_isBenchmarking, ("Only in benchmarking mode!"));
  m_benchmarkBarrier.DecrementTileCounter(sequenceID);
  m_windowHandle->invalidate();
}

void CoverageGenerator::WaitForEmptyAndFinished()
{
  m_queue.Join();
}

ScreenCoverage * CoverageGenerator::CurrentCoverage()
{
  return m_currentCoverage;
}

threads::Mutex & CoverageGenerator::Mutex()
{
  return m_mutex;
}

void CoverageGenerator::SetSequenceID(int sequenceID)
{
  m_sequenceID = sequenceID;
}

void CoverageGenerator::StartTileDrawingSession(int sequenceID, unsigned tileCount)
{
  ASSERT(m_isBenchmarking, ("Only in benchmarking mode!"));
  m_benchmarkBarrier.m_sequenceID = sequenceID;
  m_benchmarkBarrier.m_tilesCount = tileCount;
}

shared_ptr<graphics::ResourceManager> const & CoverageGenerator::resourceManager() const
{
  return m_resourceManager;
}

storage::TIndex CoverageGenerator::GetCountryIndex(m2::PointD const & pt) const
{
  return m_countryIndexFn(pt);
}

void CoverageGenerator::CancelCommands()
{
  m_queue.CancelCommands();
}

void CoverageGenerator::SetIsPaused(bool flag)
{
  m_isPaused = flag;
}

bool CoverageGenerator::DoForceUpdate() const
{
  return m_doForceUpdate;
}
