#pragma once

#include "symbol_renderer.hpp"

namespace yg
{
  class ResourceManager;
  namespace gl
  {
    class Screen : public SymbolRenderer
    {
    private:
    public:

      typedef SymbolRenderer::Params Params;

      Screen(Params const & params) : SymbolRenderer(params)
      {}
    };
  }
}
