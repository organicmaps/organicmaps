#pragma once

#include "../std/shared_ptr.hpp"

namespace graphics
{
  namespace gl
  {
    class RenderContext
    {
    public:
      virtual ~RenderContext() {}
      /// Make this context current for the specified thread
      virtual void makeCurrent() = 0;
      /// Create a render context which is shared with this one.
      virtual shared_ptr<RenderContext> createShared() = 0;
      /// called at the end of thread
      virtual void endThreadDrawing();
      /// !! IMPORTANT !!
      /// this function must be called from each opengl
      /// thread to setup texture related params
      static void initParams();
    };
  }
}
