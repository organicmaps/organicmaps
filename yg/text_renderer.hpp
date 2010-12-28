#pragma once

#include "geometry_batcher.hpp"
#include "../std/shared_ptr.hpp"

namespace yg
{
  class ResourceManager;
  namespace gl
  {
    class TextRenderer : public GeometryBatcher
    {
    private:
    public:

      typedef GeometryBatcher::Params Params;

      TextRenderer(Params const & params) : GeometryBatcher(params)
      {}
    };
  }
}
