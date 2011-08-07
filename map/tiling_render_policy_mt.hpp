#pragma once

#include "render_policy.hpp"
#include "render_queue.hpp"
#include "coverage_generator.hpp"
#include "tiler.hpp"

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

  RenderQueue m_renderQueue;
  CoverageGenerator m_coverageGenerator;

  yg::InfoLayer m_infoLayer;

//  yg::Tiler m_tiler;

public:

  TilingRenderPolicyMT(shared_ptr<WindowHandle> const & windowHandle,
                       RenderPolicy::TRenderFn const & renderFn);

  void Initialize(shared_ptr<yg::gl::RenderContext> const & renderContext,
                  shared_ptr<yg::ResourceManager> const & resourceManager);

  void OnSize(int w, int h);

  void DrawFrame(shared_ptr<PaintEvent> const & ev, ScreenBase const & currentScreen);
};
