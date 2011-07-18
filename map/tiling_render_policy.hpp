#pragma once

#include "render_policy.hpp"

class TilingRenderPolicy : public RenderPolicy
{
public:

  virtual void renderTile() = 0;

  void DrawFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s);
};
