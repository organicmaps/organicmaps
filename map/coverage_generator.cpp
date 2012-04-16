#include "../base/SRC_FIRST.hpp"

#include "coverage_generator.hpp"
#include "screen_coverage.hpp"
#include "tile_renderer.hpp"
#include "tile_set.hpp"

#include "../yg/rendercontext.hpp"

#include "../base/logging.hpp"

#include "../std/bind.hpp"

bool g_coverageGeneratorDestroyed = false;

CoverageGenerator::CoverageGenerator(
    TileRenderer * tileRenderer,
    shared_ptr<WindowHandle> const & windowHandle,
    shared_ptr<yg::gl::RenderContext> const & primaryRC,
    shared_ptr<yg::ResourceManager> const & rm,
    yg::gl::PacketsQueue * glQueue,
    RenderPolicy::TEmptyModelFn emptyModelFn)
  : m_queue(1),
    m_tileRenderer(tileRenderer),
    m_workCoverage(0),
    m_currentCoverage(new ScreenCoverage(tileRenderer, this)),
    m_sequenceID(0),
    m_windowHandle(windowHandle),
    m_emptyModelFn(emptyModelFn)
{
  g_coverageGeneratorDestroyed = false;

  m_resourceManager = rm;
  if (!glQueue)
    m_renderContext = primaryRC->createShared();

  m_currentStylesCache.reset(new yg::ResourceStyleCache(rm,
                                                 rm->cacheThreadGlyphCacheID(),
                                                 glQueue));

  m_workStylesCache.reset(new yg::ResourceStyleCache(rm,
                                              rm->cacheThreadGlyphCacheID(),
                                              glQueue));

  m_queue.AddInitCommand(bind(&CoverageGenerator::InitializeThreadGL, this));
  m_queue.AddFinCommand(bind(&CoverageGenerator::FinalizeThreadGL, this));

  m_queue.Start();
}

void CoverageGenerator::InitializeThreadGL()
{
  if (m_renderContext)
    m_renderContext->makeCurrent();
}

void CoverageGenerator::FinalizeThreadGL()
{
  if (m_renderContext)
    m_renderContext->endThreadDrawing();
}

CoverageGenerator::~CoverageGenerator()
{
  LOG(LINFO, ("cancelling coverage thread"));
  Cancel();

  LOG(LINFO, ("deleting workCoverage"));
  delete m_workCoverage;

  LOG(LINFO, ("deleting currentCoverage"));
  delete m_currentCoverage;

  g_coverageGeneratorDestroyed = true;
}

void CoverageGenerator::Cancel()
{
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
  /// this automatically will skip the previous CoverScreen commands
  /// and MergeTiles commands from previously generated ScreenCoverages
  if (m_sequenceID == numeric_limits<int>::max())
    return;

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
  m_queue.AddCommand(bind(&CoverageGenerator::CoverScreen, this, screen, m_sequenceID));
}

void CoverageGenerator::CoverScreen(ScreenBase const & screen, int sequenceID)
{
  if (sequenceID < m_sequenceID)
    return;

  ASSERT(m_workCoverage == 0, ());

  m_workCoverage = m_currentCoverage->Clone();
  m_workCoverage->SetSequenceID(sequenceID);
  m_workCoverage->SetResourceStyleCache(m_workStylesCache.get());
  m_workCoverage->SetScreen(screen);

  if (!m_workCoverage->IsPartialCoverage() && m_workCoverage->IsEmptyDrawingCoverage())
    AddCheckEmptyModelTask(sequenceID);

  m_workCoverage->CacheInfoLayer();

  {
    threads::MutexGuard g(m_mutex);

    /// test that m_workCoverage->InfoLayer doesn't have
    /// the same elements as m_currentCoverage->InfoLayer
    ASSERT(!m_workCoverage->GetInfoLayer()->checkHasEquals(m_currentCoverage->GetInfoLayer()), ());
    ASSERT(m_workCoverage->GetInfoLayer()->checkCached(m_workStylesCache.get()), ());

    swap(m_currentCoverage, m_workCoverage);
    swap(m_currentStylesCache, m_workStylesCache);

    ASSERT(m_currentCoverage->GetResourceStyleCache() == m_currentStylesCache.get(), ());
  }

  delete m_workCoverage;
  m_workCoverage = 0;

  m_windowHandle->invalidate();
}

void CoverageGenerator::AddMergeTileTask(Tiler::RectInfo const & rectInfo,
                                         int sequenceID)
{
  if (g_coverageGeneratorDestroyed)
    return;

  m_queue.AddCommand(bind(&CoverageGenerator::MergeTile, this, rectInfo, sequenceID));
}

void CoverageGenerator::MergeTile(Tiler::RectInfo const & rectInfo, int sequenceID)
{
  if (sequenceID < m_sequenceID)
  {
    m_tileRenderer->RemoveActiveTile(rectInfo);
    return;
  }

  ASSERT(m_workCoverage == 0, ());

  m_workCoverage = m_currentCoverage->Clone();
  m_workCoverage->SetSequenceID(sequenceID);
  m_workCoverage->SetResourceStyleCache(m_workStylesCache.get());
  m_workCoverage->Merge(rectInfo);

  if (!m_workCoverage->IsPartialCoverage() && m_workCoverage->IsEmptyDrawingCoverage())
    AddCheckEmptyModelTask(sequenceID);

  m_workCoverage->CacheInfoLayer();

  {
    threads::MutexGuard g(m_mutex);

    /// test that m_workCoverage->InfoLayer doesn't have
    /// the same elements as m_currentCoverage->InfoLayer
    ASSERT(!m_workCoverage->GetInfoLayer()->checkHasEquals(m_currentCoverage->GetInfoLayer()), ());
    ASSERT(m_workCoverage->GetInfoLayer()->checkCached(m_workStylesCache.get()), ());

    swap(m_currentCoverage, m_workCoverage);
    swap(m_currentStylesCache, m_workStylesCache);

    ASSERT(m_currentCoverage->GetResourceStyleCache() == m_currentStylesCache.get(), ());
  }

  /// we should delete workCoverage
  delete m_workCoverage;
  m_workCoverage = 0;

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

void CoverageGenerator::WaitForEmptyAndFinished()
{
  m_queue.Join();
}

ScreenCoverage & CoverageGenerator::CurrentCoverage()
{
  return *m_currentCoverage;
}

threads::Mutex & CoverageGenerator::Mutex()
{
  return m_mutex;
}

void CoverageGenerator::SetSequenceID(int sequenceID)
{
  m_sequenceID = sequenceID;
}

shared_ptr<yg::ResourceManager> const & CoverageGenerator::resourceManager() const
{
  return m_resourceManager;
}

bool CoverageGenerator::IsEmptyModelAtPoint(m2::PointD const & pt) const
{
  return m_emptyModelFn(pt);
}
