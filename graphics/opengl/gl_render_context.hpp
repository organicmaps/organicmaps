#pragma once

#include "../render_context.hpp"

namespace graphics
{
  namespace gl
  {
    class RenderContext : public graphics::RenderContext
    {
    public:
      void startThreadDrawing();
      void endThreadDrawing();
    };
  }
}
