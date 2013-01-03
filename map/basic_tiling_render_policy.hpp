#pragma once

#include "render_policy.hpp"

#include "../graphics/overlay.hpp"

#include "../geometry/screenbase.hpp"

#include "../std/shared_ptr.hpp"


class TileRenderer;
class CoverageGenerator;
class QueuedRenderer;

/// This is a base class for a single-threaded and multi-threaded versions of render policies
/// which uses tiles to present a scene.
/// This policies use ScreenCoverage objects to hold all the information, need to render the model
/// at the specified ScreenBase(tiles, texts, symbols, e.t.c)
/// At any moment of time there are a currentCoverage that are drawn in DrawFrame method.
/// There are threads that are responsible for composing new ScreenCoverage from already drawn tiles,
/// and drawing tiles if needed.
/// Once the more recent ScreenCoverage are composed it became a currentCoverage.
class BasicTilingRenderPolicy : public RenderPolicy
{
private:

  size_t CalculateTileSize(size_t screenWidth, size_t screenHeight);

protected:

  shared_ptr<QueuedRenderer> m_QueuedRenderer;
  shared_ptr<TileRenderer> m_TileRenderer;
  shared_ptr<CoverageGenerator> m_CoverageGenerator;

  ScreenBase m_CurrentScreen;
  int  m_DrawScale;
  bool m_IsEmptyModel;
  storage::TIndex m_countryIndex;
  bool m_DoRecreateCoverage;
  bool m_IsNavigating;
  bool m_WasAnimatingLastFrame;
  size_t m_TileSize;

protected:

  TileRenderer & GetTileRenderer();

  void StartNavigation();
  void StopNavigation();

  void PauseBackgroundRendering();
  void ResumeBackgroundRendering();
  void CheckAnimationTransition();

public:

  BasicTilingRenderPolicy(Params const & p,
                          bool doUseQueuedRenderer);

  void BeginFrame(shared_ptr<PaintEvent> const & ev, ScreenBase const & s);
  void DrawFrame(shared_ptr<PaintEvent> const & ev, ScreenBase const & s);
  void EndFrame(shared_ptr<PaintEvent> const & ev, ScreenBase const & s);

  virtual void StartScale();
  virtual void StopScale();

  virtual void StartDrag();
  virtual void StopDrag();

  virtual void StartRotate(double a, double timeInSec);
  virtual void StopRotate(double a, double timeInSec);

  bool NeedRedraw() const;
  bool IsTiling() const;
  bool IsEmptyModel() const;
  storage::TIndex GetCountryIndex() const;
  int GetDrawScale(ScreenBase const & s) const;
  size_t ScaleEtalonSize() const;
  size_t TileSize() const;

  void FrameLock();
  void FrameUnlock();
  shared_ptr<graphics::Overlay> const FrameOverlay() const;

  /// benchmarking protocol
  int InsertBenchmarkFence();
  void JoinBenchmarkFence(int fenceID);
};
