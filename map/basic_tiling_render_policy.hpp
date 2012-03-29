#pragma once

#include "render_policy.hpp"

#include "../std/shared_ptr.hpp"
#include "../yg/info_layer.hpp"

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
protected:

  shared_ptr<QueuedRenderer> m_QueuedRenderer;
  shared_ptr<TileRenderer> m_TileRenderer;
  shared_ptr<CoverageGenerator> m_CoverageGenerator;

  ScreenBase m_CurrentScreen;
  int  m_DrawScale;
  bool m_IsEmptyModel;
  bool m_DoRecreateCoverage;

protected:

  TileRenderer & GetTileRenderer();

public:

  BasicTilingRenderPolicy(shared_ptr<yg::gl::RenderContext> const & primaryRC,
                          bool doSupportRotation,
                          size_t idCacheSize,
                          shared_ptr<QueuedRenderer> const & queuedRenderer = shared_ptr<QueuedRenderer>());

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
  int  GetDrawScale(ScreenBase const & s) const;
  size_t ScaleEtalonSize() const;
};
