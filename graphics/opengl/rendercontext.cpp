#include "../base/SRC_FIRST.hpp"
#include "rendercontext.hpp"
#include "opengl.hpp"

namespace graphics
{
  namespace gl
  {
    void RenderContext::initParams()
    {
      OGLCHECK(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
      OGLCHECK(glPixelStorei(GL_PACK_ALIGNMENT, 1));
      graphics::gl::InitializeThread();
    }

    void RenderContext::endThreadDrawing()
    {
      graphics::gl::FinalizeThread();
    }
  }
}
