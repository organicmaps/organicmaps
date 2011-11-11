#include "../base/SRC_FIRST.hpp"
#include "benchmark_tiling_render_policy_mt.hpp"

BenchmarkTilingRenderPolicyMT::BenchmarkTilingRenderPolicyMT(
    VideoTimer * videoTimer,
    DrawerYG::Params const & params,
    shared_ptr<yg::gl::RenderContext> const & primaryRC)
  : TilingRenderPolicyMT(videoTimer, params, primaryRC)
{}

void BenchmarkTilingRenderPolicyMT::DrawFrame(shared_ptr<PaintEvent> const & e,
                                              ScreenBase const & s)
{
  TilingRenderPolicyMT::DrawFrame(e, s);
  /// waiting for render queue to empty
  GetTileRenderer().WaitForEmptyAndFinished();
  /// current screen should be fully covered, so the following call will only blit results
  TilingRenderPolicyMT::DrawFrame(e, s);
}

