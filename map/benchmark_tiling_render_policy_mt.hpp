#pragma once

#include "tiling_render_policy_mt.hpp"

class BenchmarkTilingRenderPolicyMT : public TilingRenderPolicyMT
{
public:

  BenchmarkTilingRenderPolicyMT(shared_ptr<WindowHandle> const & wh,
                              RenderPolicy::TRenderFn const & renderFn);

  void Initialize(shared_ptr<yg::gl::RenderContext> const & renderContext,
                  shared_ptr<yg::ResourceManager> const & resourceManager);

  void DrawFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s);

  void OnSize(int w, int h);
};
