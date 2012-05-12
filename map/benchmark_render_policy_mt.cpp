#include "../base/SRC_FIRST.hpp"

#include "benchmark_render_policy_mt.hpp"

#include "render_queue.hpp"

#include "../base/logging.hpp"

BenchmarkRenderPolicyMT::BenchmarkRenderPolicyMT(Params const & p)
  : RenderPolicyMT(p)
{}

void BenchmarkRenderPolicyMT::DrawFrame(shared_ptr<PaintEvent> const & e,
                                        ScreenBase const & s)
{
  RenderPolicyMT::DrawFrame(e, s);
  RenderPolicyMT::EndFrame(e, s);
  GetRenderQueue().WaitForEmptyAndFinished();
  RenderPolicyMT::BeginFrame(e, s);
  RenderPolicyMT::DrawFrame(e, s);
}


