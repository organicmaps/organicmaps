#pragma once

#include "tiling_render_policy_mt.hpp"

class BenchmarkTilingRenderPolicyMT : public TilingRenderPolicyMT
{
public:

  BenchmarkTilingRenderPolicyMT(Params const & p);

  void DrawFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s);
};
