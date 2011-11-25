#pragma once

#include "render_policy.hpp"
#include "tile_renderer.hpp"
#include "coverage_generator.hpp"
#include "tiler.hpp"
#include "screen_coverage.hpp"

#include "../yg/info_layer.hpp"
#include "../std/scoped_ptr.hpp"

namespace yg
{
  namespace gl
  {
    class RenderContext;
  }
  class ResourceManager;
}

class WindowHandle;

class TilingRenderPolicyMT : public RenderPolicy
{
private:

  scoped_ptr<TileRenderer> m_tileRenderer;

  scoped_ptr<CoverageGenerator> m_coverageGenerator;

  ScreenBase m_currentScreen;

  bool m_isScaling;

//  ScreenCoverage m_screenCoverage;

protected:

  TileRenderer & GetTileRenderer();

public:

  TilingRenderPolicyMT(VideoTimer * videoTimer,
                       bool useDefaultFB,
                       yg::ResourceManager::Params const & rmParams,
                       shared_ptr<yg::gl::RenderContext> const & primaryRC);

  void BeginFrame(shared_ptr<PaintEvent> const & ev, ScreenBase const & s);
  void DrawFrame(shared_ptr<PaintEvent> const & ev, ScreenBase const & s);
  void EndFrame(shared_ptr<PaintEvent> const & ev, ScreenBase const & s);

  virtual void StartScale();
  virtual void StopScale();

  bool IsTiling() const;

  void SetRenderFn(TRenderFn renderFn);
};
