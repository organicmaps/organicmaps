#pragma once

#include "render_policy.hpp"

class SimpleRenderPolicy : public RenderPolicy
{
public:
  SimpleRenderPolicy(Params const & p);

  void DrawFrame(shared_ptr<PaintEvent> const & paintEvent,
                 ScreenBase const & screenBase);
};
