#include "basic_tiling_render_policy.hpp"

#include "../platform/platform.hpp"

#include "../indexer/scales.hpp"

#include "tile_renderer.hpp"
#include "coverage_generator.hpp"
#include "screen_coverage.hpp"
#include "queued_renderer.hpp"

size_t BasicTilingRenderPolicy::CalculateTileSize(size_t screenWidth, size_t screenHeight)
{
  int const maxSz = max(screenWidth, screenHeight);

  // we're calculating the tileSize based on (maxSz > 1024 ? rounded : ceiled)
  // to the nearest power of two value of the maxSz

  int const ceiledSz = 1 << static_cast<int>(ceil(log(double(maxSz + 1)) / log(2.0)));
  int res = 0;

  if (maxSz < 1024)
    res = ceiledSz;
  else
  {
    int const flooredSz = ceiledSz / 2;
    // rounding to the nearest power of two.
    if (ceiledSz - maxSz < maxSz - flooredSz)
      res = ceiledSz;
    else
      res = flooredSz;
  }

  return min(max(res / 2, 128), 1024);
}

BasicTilingRenderPolicy::BasicTilingRenderPolicy(Params const & p,
                                                 bool doUseQueuedRenderer)
  : RenderPolicy(p, true, GetPlatform().CpuCores()),
    m_DrawScale(0),
    m_IsEmptyModel(false),
    m_DoRecreateCoverage(false),
    m_IsNavigating(false),
    m_WasAnimatingLastFrame(false)
{
  m_TileSize = CalculateTileSize(p.m_screenWidth, p.m_screenHeight);

  LOG(LINFO, ("ScreenSize=", p.m_screenWidth, "x", p.m_screenHeight, ", TileSize=", m_TileSize));

  if (doUseQueuedRenderer)
    m_QueuedRenderer.reset(new QueuedRenderer(GetPlatform().CpuCores() + 1));
}

void BasicTilingRenderPolicy::BeginFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s)
{
  RenderPolicy::BeginFrame(e, s);

  if (m_QueuedRenderer)
    m_QueuedRenderer->BeginFrame();
}

void BasicTilingRenderPolicy::CheckAnimationTransition()
{
  // transition from non-animating to animating,
  // should stop all background work
  if (!m_WasAnimatingLastFrame && IsAnimating())
    PauseBackgroundRendering();

  // transition from animating to non-animating
  // should resume all background work
  if (m_WasAnimatingLastFrame && !IsAnimating())
    ResumeBackgroundRendering();

  m_WasAnimatingLastFrame = IsAnimating();
}

void BasicTilingRenderPolicy::DrawFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s)
{
  if (m_QueuedRenderer)
  {
    m_QueuedRenderer->DrawFrame();
    m_resourceManager->updatePoolState();
  }

  CheckAnimationTransition();

  /// checking, whether we should add the CoverScreen command

  bool doForceUpdate = DoForceUpdate();
  bool doIntersectInvalidRect = GetInvalidRect().IsIntersect(s.GlobalRect());

  if (doForceUpdate)
    m_CoverageGenerator->InvalidateTiles(GetInvalidRect(), scales::GetUpperWorldScale() + 1);

  if (!m_IsNavigating && (!IsAnimating()))
    m_CoverageGenerator->AddCoverScreenTask(s,
                                            m_DoRecreateCoverage || (doForceUpdate && doIntersectInvalidRect));

  SetForceUpdate(false);
  m_DoRecreateCoverage = false;

  /// rendering current coverage

  DrawerYG * pDrawer = e->drawer();

  pDrawer->beginFrame();

  pDrawer->screen()->clear(m_bgColor);

  m_CoverageGenerator->Mutex().Lock();

  ScreenCoverage * curCvg = &m_CoverageGenerator->CurrentCoverage();

  SetOverlay(curCvg->GetOverlay());

  curCvg->Draw(pDrawer->screen().get(), s);

  m_DrawScale = curCvg->GetDrawScale();

  if (!curCvg->IsEmptyDrawingCoverage() || !curCvg->IsPartialCoverage())
  {
    m_IsEmptyModel = curCvg->IsEmptyDrawingCoverage() && curCvg->IsEmptyModelAtCoverageCenter();
    if (m_IsEmptyModel)
      m_CountryName = curCvg->GetCountryNameAtCoverageCenter();
  }

  pDrawer->endFrame();
}

void BasicTilingRenderPolicy::EndFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s)
{
  m_CoverageGenerator->Mutex().Unlock();

  if (m_QueuedRenderer)
    m_QueuedRenderer->EndFrame();
}

TileRenderer & BasicTilingRenderPolicy::GetTileRenderer()
{
  return *m_TileRenderer.get();
}

void BasicTilingRenderPolicy::PauseBackgroundRendering()
{
  m_TileRenderer->SetIsPaused(true);
  m_TileRenderer->CancelCommands();
  if (m_QueuedRenderer)
    m_QueuedRenderer->SetPartialExecution(GetPlatform().CpuCores(), true);
}

void BasicTilingRenderPolicy::ResumeBackgroundRendering()
{
  m_TileRenderer->SetIsPaused(false);
  m_DoRecreateCoverage = true;
  if (m_QueuedRenderer)
    m_QueuedRenderer->SetPartialExecution(GetPlatform().CpuCores(), false);
}

void BasicTilingRenderPolicy::StartNavigation()
{
  m_IsNavigating = true;
  PauseBackgroundRendering();
}

void BasicTilingRenderPolicy::StopNavigation()
{
  m_IsNavigating = false;
  ResumeBackgroundRendering();
}

void BasicTilingRenderPolicy::StartScale()
{
  StartNavigation();
}

void BasicTilingRenderPolicy::StopScale()
{
  StopNavigation();
  RenderPolicy::StopScale();
}

void BasicTilingRenderPolicy::StartDrag()
{
  StartNavigation();
}

void BasicTilingRenderPolicy::StopDrag()
{
  StopNavigation();
  RenderPolicy::StopDrag();
}

void BasicTilingRenderPolicy::StartRotate(double a, double timeInSec)
{
  StartNavigation();
}

void BasicTilingRenderPolicy::StopRotate(double a, double timeInSec)
{
  StopNavigation();
  RenderPolicy::StopRotate(a, timeInSec);
}

bool BasicTilingRenderPolicy::IsTiling() const
{
  return true;
}

int BasicTilingRenderPolicy::GetDrawScale(ScreenBase const & s) const
{
  return m_DrawScale;
}

bool BasicTilingRenderPolicy::IsEmptyModel() const
{
  return m_IsEmptyModel;
}

string const BasicTilingRenderPolicy::GetCountryName() const
{
  return m_CountryName;
}

bool BasicTilingRenderPolicy::NeedRedraw() const
{
  if (RenderPolicy::NeedRedraw())
    return true;

  if (m_QueuedRenderer && m_QueuedRenderer->NeedRedraw())
    return true;

  return false;
}

size_t BasicTilingRenderPolicy::ScaleEtalonSize() const
{
  return m_TileSize;
}

size_t BasicTilingRenderPolicy::TileSize() const
{
  return m_TileSize;
}

int BasicTilingRenderPolicy::InsertBenchmarkFence()
{
  return m_CoverageGenerator->InsertBenchmarkFence();
}

void BasicTilingRenderPolicy::JoinBenchmarkFence(int fenceID)
{
  return m_CoverageGenerator->JoinBenchmarkFence(fenceID);
}
