#include "../base/SRC_FIRST.hpp"
#include "../platform/settings.hpp"

#include "coverage_generator.hpp"
#include "screen_coverage.hpp"
#include "tile_renderer.hpp"
#include "tile_set.hpp"

#include "../graphics/opengl/gl_render_context.hpp"

#include "../base/logging.hpp"

#include "../std/bind.hpp"

CoverageGenerator::CoverageGenerator(
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
    m_windowHandle(windowHandle),
    m_countryIndexFn(countryIndexFn),
    m_glQueue(glQueue)
{
  m_resourceManager = rm;

  if (!m_glQueue)
    m_renderContext.reset(primaryRC->createShared());

  m_queue.AddInitCommand(bind(&CoverageGenerator::InitializeThreadGL, this));
  m_queue.AddFinCommand(bind(&CoverageGenerator::FinalizeThreadGL, this));

  m_queue.Start();

  Settings::Get("IsBenchmarking", m_benchmarkInfo.m_isBenchmarking);
}

CoverageGenerator::~CoverageGenerator()
{
  LOG(LINFO, ("cancelling coverage thread"));
  m_queue.Cancel();

  LOG(LINFO, ("deleting workCoverage"));
  delete m_workCoverage;
  m_workCoverage = 0;

  LOG(LINFO, ("deleting currentCoverage"));
  delete m_currentCoverage;
  m_currentCoverage = 0;
}

void CoverageGenerator::Shutdown()
{
  m_stateInfo.SetSequenceID(numeric_limits<int>::max());
  m_queue.Join();
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

  shared_ptr<graphics::Screen> screen(new graphics::Screen(params));

  ScreenCoverage * screenCoverage = new ScreenCoverage(m_tileRenderer, this, screen);
  screenCoverage->SetBenchmarkingFlag(m_benchmarkInfo.m_isBenchmarking);

  return screenCoverage;
}

void CoverageGenerator::InitializeThreadGL()
{
  threads::MutexGuard g(m_stateInfo.m_mutex);

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
  if ((screen == m_stateInfo.m_currentScreen) && (!doForce))
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

void CoverageGenerator::CheckEmptyModel(int sequenceID)
{
  m_queue.AddCommand(bind(&CoverageGenerator::CheckEmptyModelImpl, this, sequenceID));
}

void CoverageGenerator::FinishSequenceIfNeeded()
{
  m_queue.AddCommand(bind(&CoverageGenerator::BenchmarkInfo::TryFinishSequence, &m_benchmarkInfo));
}

void CoverageGenerator::DecrementTileCount(int sequenceID)
{
  m_queue.AddCommand(bind(&CoverageGenerator::BenchmarkInfo::DecrementTileCount, &m_benchmarkInfo, sequenceID));
}

ScreenCoverage * CoverageGenerator::CurrentCoverage()
{
  return m_currentCoverage;
}

void CoverageGenerator::Lock()
{
  m_stateInfo.m_mutex.Lock();
}

void CoverageGenerator::Unlock()
{
  m_stateInfo.m_mutex.Unlock();
}

void CoverageGenerator::StartTileDrawingSession(int sequenceID, unsigned tileCount)
{
  ASSERT(m_benchmarkInfo.m_isBenchmarking, ("Only in benchmarking mode!"));
  m_benchmarkInfo.m_benchmarkSequenceID = sequenceID;
  m_benchmarkInfo.m_tilesCount = tileCount;
}

storage::TIndex CoverageGenerator::GetCountryIndex(m2::PointD const & pt) const
{
  return m_countryIndexFn(pt);
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

  m_currentCoverage->CopyInto(*m_workCoverage, false);

  m_workCoverage->SetSequenceID(sequenceID);
  m_workCoverage->SetScreen(screen);

  if (!m_workCoverage->IsPartialCoverage() && m_workCoverage->IsEmptyDrawingCoverage())
  {
    m_workCoverage->ResetEmptyModelAtCoverageCenter();
    CheckEmptyModel(sequenceID);
  }

  bool shouldSwap = !m_stateInfo.m_isPause && m_workCoverage->Cache(env);

  if (shouldSwap)
  {
    threads::MutexGuard g(m_stateInfo.m_mutex);
    swap(m_currentCoverage, m_workCoverage);
  }
  else
  {
    /// we should skip all the following MergeTile commands
    ++m_stateInfo.m_sequenceID;
  }

  m_stateInfo.SetForceUpdate(!shouldSwap);

  m_workCoverage->Clear();

  m_windowHandle->invalidate();
}

void CoverageGenerator::MergeTileImpl(core::CommandsQueue::Environment const & env,
                                  Tiler::RectInfo const & rectInfo,
                                  int sequenceID)
{
  if (sequenceID < m_stateInfo.m_sequenceID)
  {
    m_tileRenderer->RemoveActiveTile(rectInfo, sequenceID);
    return;
  }

  m_currentCoverage->CopyInto(*m_workCoverage, true);
  m_workCoverage->SetSequenceID(sequenceID);
  m_workCoverage->Merge(rectInfo);

  if (!m_workCoverage->IsPartialCoverage() && m_workCoverage->IsEmptyDrawingCoverage())
  {
    m_workCoverage->ResetEmptyModelAtCoverageCenter();
    CheckEmptyModel(sequenceID);
  }

  bool shouldSwap = !m_stateInfo.m_isPause && m_workCoverage->Cache(env);

  if (shouldSwap)
  {
    threads::MutexGuard g(m_stateInfo.m_mutex);
    swap(m_currentCoverage, m_workCoverage);
    m_benchmarkInfo.DecrementTileCount(sequenceID);
  }

  m_stateInfo.SetForceUpdate(!shouldSwap);

  m_workCoverage->Clear();

  if (!m_benchmarkInfo.m_isBenchmarking)
    m_windowHandle->invalidate();
}

void CoverageGenerator::InvalidateTilesImpl(m2::AnyRectD const & r, int startScale)
{
  {
    threads::MutexGuard g(m_stateInfo.m_mutex);
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

void CoverageGenerator::CheckEmptyModelImpl(int sequenceID)
{
  if (sequenceID < m_stateInfo.m_sequenceID)
    return;

  m_currentCoverage->CheckEmptyModelAtCoverageCenter();

  m_windowHandle->invalidate();
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
///             BenchmarkInfo
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
