#pragma once

#include "queued_render_policy.hpp"
#include "tile_renderer.hpp"
#include "coverage_generator.hpp"
#include "tiler.hpp"
#include "screen_coverage.hpp"
#include "render_policy.hpp"

#include "../yg/info_layer.hpp"
#include "../std/scoped_ptr.hpp"

namespace yg
{
  namespace gl
  {
    class Screen;
    class RenderContext;
  }
  class ResourceManager;
}

class WindowHandle;

class TilingRenderPolicyST : public QueuedRenderPolicy
{
private:

  typedef QueuedRenderPolicy base_t;

  scoped_ptr<TileRenderer> m_tileRenderer;

  scoped_ptr<CoverageGenerator> m_coverageGenerator;

  ScreenBase m_currentScreen;

  bool m_isScaling;
  int m_maxTilesCount;

  int m_drawScale;
  bool m_isEmptyModel;

  bool m_doRecreateCoverage;

protected:

  TileRenderer & GetTileRenderer();

public:

  TilingRenderPolicyST(VideoTimer * videoTimer,
                       bool useDefaultFB,
                       yg::ResourceManager::Params const & rmParams,
                       shared_ptr<yg::gl::RenderContext> const & primaryRC);

  ~TilingRenderPolicyST();

  void DrawFrame(shared_ptr<PaintEvent> const & ev, ScreenBase const & s);
  void EndFrame(shared_ptr<PaintEvent> const & ev, ScreenBase const & s);

  virtual void StartScale();
  virtual void StopScale();

  virtual void StartDrag();
  virtual void StopDrag();

  virtual void StartRotate(double a, double timeInSec);
  virtual void StopRotate(double a, double timeInSec);

  bool IsTiling() const;
  bool IsEmptyModel() const;
  int GetDrawScale(ScreenBase const & s) const;

  void SetRenderFn(TRenderFn renderFn);
};
