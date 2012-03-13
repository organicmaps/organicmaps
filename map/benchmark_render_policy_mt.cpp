#include "../base/SRC_FIRST.hpp"

#include "benchmark_render_policy_mt.hpp"

#include "render_queue.hpp"

#include "../base/logging.hpp"

BenchmarkRenderPolicyMT::BenchmarkRenderPolicyMT(VideoTimer * videoTimer,
                                                 bool useDefaultFB,
                                                 yg::ResourceManager::Params const & rmParams,
                                                 shared_ptr<yg::gl::RenderContext> const & primaryRC)
  : RenderPolicyMT(videoTimer, useDefaultFB, rmParams, primaryRC)
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


