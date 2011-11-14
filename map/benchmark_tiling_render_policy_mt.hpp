#pragma once

#include "tiling_render_policy_mt.hpp"

class BenchmarkTilingRenderPolicyMT : public TilingRenderPolicyMT
{
public:

  BenchmarkTilingRenderPolicyMT(VideoTimer * videoTimer,
                                DrawerYG::Params const & params,
                                yg::ResourceManager::Params const & rmParams,
                                shared_ptr<yg::gl::RenderContext> const & primaryRC);

  void DrawFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s);
};
