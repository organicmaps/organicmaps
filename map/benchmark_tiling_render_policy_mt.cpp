#include "../base/SRC_FIRST.hpp"
#include "benchmark_tiling_render_policy_mt.hpp"

BenchmarkTilingRenderPolicyMT::BenchmarkTilingRenderPolicyMT(
  shared_ptr<WindowHandle> const & wh,
  RenderPolicy::TRenderFn const & renderFn)
  : TilingRenderPolicyMT(wh, renderFn)
{}

void BenchmarkTilingRenderPolicyMT::Initialize(shared_ptr<yg::gl::RenderContext> const & renderContext,
                                               shared_ptr<yg::ResourceManager> const & resourceManager)
{
  TilingRenderPolicyMT::Initialize(renderContext, resourceManager);
}

void BenchmarkTilingRenderPolicyMT::OnSize(int w, int h)
{
  TilingRenderPolicyMT::OnSize(w, h);
}

void BenchmarkTilingRenderPolicyMT::DrawFrame(shared_ptr<PaintEvent> const & e,
                                              ScreenBase const & s)
{
  TilingRenderPolicyMT::DrawFrame(e, s);
  /// waiting for render queue to empty
  GetTileRenderer().WaitForEmptyAndFinished();
  /// current screen should be fully covered, so the following call will only blit results
  TilingRenderPolicyMT::DrawFrame(e, s);
}

