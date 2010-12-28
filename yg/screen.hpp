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
      Screen(shared_ptr<yg::ResourceManager> const & rm, bool isAntiAliased)
        : TextRenderer(rm, isAntiAliased)
      {}
    };
  }
}
