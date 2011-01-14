#pragma once

#include "shape_renderer.hpp"
#include "../std/shared_ptr.hpp"

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
