#pragma once

#include "graphics/overlay_renderer.hpp"

namespace graphics
{
  class Screen : public OverlayRenderer
  {
  public:
    typedef OverlayRenderer::Params Params;
    Screen(Params const & params) : OverlayRenderer(params)
    {}
  };
}
