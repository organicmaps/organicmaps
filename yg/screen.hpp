#pragma once

#include "text_renderer.hpp"
#include "../std/shared_ptr.hpp"

namespace yg
{
  class ResourceManager;
  namespace gl
  {
    class Screen : public TextRenderer
    {
    private:
    public:

      typedef TextRenderer::Params Params;

      Screen(Params const & params) : TextRenderer(params)
      {}
    };
  }
}
