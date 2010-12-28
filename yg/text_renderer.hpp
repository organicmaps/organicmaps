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
      TextRenderer(shared_ptr<ResourceManager> const & rm, bool isAntiAliased)
        : GeometryBatcher(rm, isAntiAliased)
      {}
    };
  }
}
