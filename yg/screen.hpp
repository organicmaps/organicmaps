#pragma once

#include "shape_renderer.hpp"

namespace yg
{
  class ResourceManager;
  namespace gl
  {
    class Screen : public ShapeRenderer
    {
    private:
    public:

      typedef ShapeRenderer::Params Params;

      Screen(Params const & params) : ShapeRenderer(params)
      {}
    };
  }
}
