#pragma once

#include "render_policy.hpp"
#include "tile_renderer.hpp"
#include "coverage_generator.hpp"
#include "tiler.hpp"
#include "screen_coverage.hpp"

#include "../yg/info_layer.hpp"

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

  TileRenderer m_tileRenderer;

  CoverageGenerator m_coverageGenerator;

  ScreenBase m_currentScreen;

//  ScreenCoverage m_screenCoverage;

protected:

  TileRenderer & GetTileRenderer();

public:

  TilingRenderPolicyMT(shared_ptr<WindowHandle> const & windowHandle,
                       RenderPolicy::TRenderFn const & renderFn);

  void Initialize(shared_ptr<yg::gl::RenderContext> const & renderContext,
                  shared_ptr<yg::ResourceManager> const & resourceManager);

  void BeginFrame(shared_ptr<PaintEvent> const & ev, ScreenBase const & s);
  void DrawFrame(shared_ptr<PaintEvent> const & ev, ScreenBase const & s);
  void EndFrame(shared_ptr<PaintEvent> const & ev, ScreenBase const & s);
};
