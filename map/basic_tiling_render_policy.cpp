#include "basic_tiling_render_policy.hpp"

#include "../indexer/scales.hpp"

#include "tile_renderer.hpp"
#include "coverage_generator.hpp"
#include "screen_coverage.hpp"
#include "queued_renderer.hpp"

BasicTilingRenderPolicy::BasicTilingRenderPolicy(shared_ptr<yg::gl::RenderContext> const & primaryRC,
                                                 bool doSupportRotation,
                                                 size_t idCacheSize,
                                                 shared_ptr<QueuedRenderer> const & queuedRenderer)
  : RenderPolicy(primaryRC, doSupportRotation, idCacheSize),
    m_QueuedRenderer(queuedRenderer),
    m_DrawScale(0),
    m_IsEmptyModel(false),
    m_DoRecreateCoverage(false)
{
}

void BasicTilingRenderPolicy::BeginFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s)
{
  if (m_QueuedRenderer)
    m_QueuedRenderer->BeginFrame();
}

void BasicTilingRenderPolicy::DrawFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s)
{
  if (m_QueuedRenderer)
  {
    m_QueuedRenderer->DrawFrame();
    m_resourceManager->updatePoolState();
  }

  /// checking, whether we should add the CoverScreen command

  bool doForceUpdate = DoForceUpdate();
  bool doIntersectInvalidRect = GetInvalidRect().IsIntersect(s.GlobalRect());

  if (doForceUpdate)
    m_CoverageGenerator->InvalidateTiles(GetInvalidRect(), scales::GetUpperWorldScale() + 1);

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

  curCvg->Draw(pDrawer->screen().get(), s);

  m_DrawScale = curCvg->GetDrawScale();

  if (!curCvg->IsEmptyDrawingCoverage() || !curCvg->IsPartialCoverage())
    m_IsEmptyModel = curCvg->IsEmptyDrawingCoverage();

  pDrawer->endFrame();
}

void BasicTilingRenderPolicy::EndFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s)
{
  ScreenCoverage * curCvg = &m_CoverageGenerator->CurrentCoverage();

  DrawerYG * pDrawer = e->drawer();

  curCvg->EndFrame(pDrawer->screen().get());
  m_CoverageGenerator->Mutex().Unlock();

  if (m_QueuedRenderer)
    m_QueuedRenderer->EndFrame();
}

TileRenderer & BasicTilingRenderPolicy::GetTileRenderer()
{
  return *m_TileRenderer.get();
}

void BasicTilingRenderPolicy::StartScale()
{
  m_TileRenderer->SetIsPaused(true);
  m_TileRenderer->CancelCommands();
}

void BasicTilingRenderPolicy::StopScale()
{
  m_TileRenderer->SetIsPaused(false);
  m_DoRecreateCoverage = true;
  RenderPolicy::StopScale();
}

void BasicTilingRenderPolicy::StartDrag()
{
  m_TileRenderer->SetIsPaused(true);
  m_TileRenderer->CancelCommands();
}

void BasicTilingRenderPolicy::StopDrag()
{
  m_TileRenderer->SetIsPaused(false);
  m_DoRecreateCoverage = true;
  RenderPolicy::StopDrag();
}

void BasicTilingRenderPolicy::StartRotate(double a, double timeInSec)
{
  m_TileRenderer->SetIsPaused(true);
  m_TileRenderer->CancelCommands();
}

void BasicTilingRenderPolicy::StopRotate(double a, double timeInSec)
{
  m_TileRenderer->SetIsPaused(false);
  m_DoRecreateCoverage = true;
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

bool BasicTilingRenderPolicy::NeedRedraw() const
{
  if (RenderPolicy::NeedRedraw())
    return true;

  if (m_QueuedRenderer && m_QueuedRenderer->NeedRedraw())
    return true;

  return false;
}

