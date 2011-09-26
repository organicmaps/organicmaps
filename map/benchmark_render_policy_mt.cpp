#include "../base/SRC_FIRST.hpp"

#include "benchmark_render_policy_mt.hpp"

#include "../base/logging.hpp"

BenchmarkRenderPolicyMT::BenchmarkRenderPolicyMT(shared_ptr<WindowHandle> const & wh,
                                                 RenderPolicy::TRenderFn const & renderFn)
  : RenderPolicyMT(wh, renderFn)
{}

void BenchmarkRenderPolicyMT::Initialize(shared_ptr<yg::gl::RenderContext> const & rc,
                                         shared_ptr<yg::ResourceManager> const & rm)
{
  RenderPolicyMT::Initialize(rc, rm);
}

void BenchmarkRenderPolicyMT::DrawFrame(shared_ptr<PaintEvent> const & e,
                                        ScreenBase const & s)
{
  RenderPolicyMT::DrawFrame(e, s);
  RenderPolicyMT::EndFrame(e, s);
  GetRenderQueue().WaitForEmptyAndFinished();
  RenderPolicyMT::BeginFrame(e, s);
  RenderPolicyMT::DrawFrame(e, s);
}


