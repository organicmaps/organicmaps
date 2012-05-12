#pragma once

#include "render_policy_mt.hpp"

class BenchmarkRenderPolicyMT : public RenderPolicyMT
{
public:
  BenchmarkRenderPolicyMT(Params const & p);

  void DrawFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s);
};
